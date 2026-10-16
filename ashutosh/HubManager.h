// HubManager.h  (Ashutosh) - RAII owner of every module. Turns each API call into engine calls.
#pragma once
#include <algorithm>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Containers.h"
#include "HashTable.h"
#include "Hub.h"
#include "Json.h"
#include "MinHeap.h"
#include "Parcel.h"
#include "Route.h"
#include "Trie.h"

struct UnknownEndpoint : std::runtime_error { using std::runtime_error::runtime_error; };

class HubManager {
public:
    explicit HubManager(std::string name)
        : hub_(std::move(name)),
          standard_(std::make_shared<StandardRate>()),
          express_(std::make_shared<ExpressSurgeRate>()) {
        // Sample places (placeholders). Replace with real hub zones later.
        const char* places[][2] = {{"248001", "Clock Tower"}, {"248002", "Clement Town"},
                                   {"248009", "Rajpur Road"}, {"248140", "Jakhan"}};
        for (const auto& p : places) trie_.insert(p[0], p[1]);
        route_.add(WAREHOUSE);
    }

    HubManager(const HubManager&) = delete;
    HubManager& operator=(const HubManager&) = delete;

    // endpoint = the part after /api/ ; body = flat JSON. Returns JSON, or throws std::runtime_error.
    std::string handle(const std::string& endpoint, const std::string& body) {
        if (endpoint == "suggest")    return suggest(body);
        if (endpoint == "quote")      return quoteCost(body);
        if (endpoint == "register")   return registerParcel(body);
        if (endpoint == "track")      return track(body);
        if (endpoint == "dispatch")   return dispatch();
        if (endpoint == "loadVan")    return loadVan();
        if (endpoint == "deliver")    return deliver();
        if (endpoint == "cancelStop") return cancelStop(body);
        if (endpoint == "state")      return state();
        throw UnknownEndpoint("Unknown endpoint: " + endpoint);
    }

private:
    static constexpr std::size_t BELT_SIZE = 5;
    static constexpr std::size_t VAN_SIZE = 5;
    static constexpr const char* WAREHOUSE = "Central Warehouse";

    Hub hub_;                                        // owns every Parcel
    Trie trie_;                                      // Step 1: destination validation
    HashTable index_;                                // Step 1: tracking ID -> Parcel*
    MinHeap queue_;                                  // Step 2: priority dispatch
    CircularQueue<Parcel*, BELT_SIZE> belt_;         // Step 2: conveyor belt
    Stack<Parcel*, VAN_SIZE> van_;                   // Step 3: van cargo
    Route route_;                                    // Step 4: driver route
    std::map<std::string, Route::Handle> stops_;     // stop name -> node, for O(1) cancel
    std::shared_ptr<const ShippingStrategy> standard_;
    std::shared_ptr<const ShippingStrategy> express_;
    unsigned long long nextId_ = 100000000101ULL;    // 12-digit tracking IDs

    static std::string number(double v) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(2) << v;
        return o.str();
    }

    std::shared_ptr<const ShippingStrategy> pickStrategy(const std::string& service) const {
        if (service == "standard") return standard_;
        if (service == "express")  return express_;
        throw std::runtime_error("Unknown service.");
    }

    static std::string parcelJson(const Parcel& p) {
        return "{\"id\":" + json::quote(p.id()) + ",\"pincode\":" + json::quote(p.pincode()) +
               ",\"dest\":" + json::quote(p.destination()) + ",\"service\":" + json::quote(p.service()) +
               ",\"priority\":" + std::to_string(p.priority()) + ",\"cost\":" + number(p.cost()) +
               ",\"status\":" + json::quote(p.statusText()) + "}";
    }

    static std::string idArray(std::vector<std::string> ids) {
        std::string out = "[";
        for (std::size_t i = 0; i < ids.size(); ++i) out += (i ? "," : "") + json::quote(ids[i]);
        return out + "]";
    }

    // ---- endpoints ----
    std::string suggest(const std::string& body) const {
        const std::string prefix = json::getString(body, "prefix");
        std::string out = "[";
        if (!prefix.empty()) {
            bool first = true;
            for (const auto& e : trie_.suggest(prefix)) {
                out += (first ? "" : ",");
                out += "{\"pin\":" + json::quote(e.pin) + ",\"name\":" + json::quote(e.name) + "}";
                first = false;
            }
        }
        return out + "]";
    }

    std::string quoteCost(const std::string& body) const {
        auto strategy = pickStrategy(json::getString(body, "service"));
        const double w = json::getNumber(body, "weight"), d = json::getNumber(body, "distance");
        if (!(w > 0) || !(d > 0)) throw std::runtime_error("Weight and distance must be positive.");
        return "{\"cost\":" + number(strategy->calcCost(w, d)) + "}";
    }

    std::string registerParcel(const std::string& body) {
        const std::string pin = json::getString(body, "pincode");
        const Trie::Entry* place = trie_.find(pin);
        if (place == nullptr || place->pin != pin) throw std::runtime_error("Unknown pincode. Pick one from the suggestions.");
        if (queue_.isFull()) throw std::runtime_error("Dispatch queue is full.");

        const std::string service = json::getString(body, "service");
        auto strategy = pickStrategy(service);
        auto parcel = std::make_unique<Parcel>(std::to_string(nextId_), place->name, pin,
                                               json::getNumber(body, "weight"), json::getNumber(body, "distance"),
                                               service == "express" ? 1 : 2, strategy);   // throws if invalid
        ++nextId_;

        Parcel* p = hub_.addParcel(std::move(parcel));
        index_.insert(p->id(), p);
        queue_.insert(p);
        if (stops_.find(p->destination()) == stops_.end()) stops_[p->destination()] = route_.add(p->destination());
        return parcelJson(*p);
    }

    std::string track(const std::string& body) const {
        Parcel* p = index_.search(json::getString(body, "id"));
        if (p == nullptr) throw std::runtime_error("No parcel with that ID.");
        return parcelJson(*p);
    }

    std::string dispatch() {
        if (belt_.isFull()) throw std::runtime_error("Belt is full.");
        Parcel* p = queue_.remove();
        if (p == nullptr) throw std::runtime_error("No parcels waiting.");
        belt_.push(p);
        p->setStatus(Status::Sorted);
        return parcelJson(*p);
    }

    std::string loadVan() {
        if (belt_.isEmpty()) throw std::runtime_error("Belt is empty.");
        if (van_.isFull()) throw std::runtime_error("Van is full.");

        // 1. Take as many parcels off the belt as the van has room for.
        std::vector<Parcel*> cargo;
        Parcel* p = nullptr;
        const std::size_t room = VAN_SIZE - van_.size();
        std::size_t loaded = 0;
        while (loaded < room && belt_.pop(p)) { cargo.push_back(p); ++loaded; }

        // 2. Re-stack: empty the van into the same list so parcels from EARLIER loads
        //    are ranked together with the new ones (otherwise a later Standard parcel
        //    would sit on top of an earlier Express one and be delivered first).
        while (van_.pop(p)) cargo.push_back(p);

        // 3. Most urgent first; arrival order breaks ties.
        std::sort(cargo.begin(), cargo.end(), [](const Parcel* a, const Parcel* b) {
            if (a->priority() != b->priority()) return a->priority() < b->priority();
            return a->sequence() < b->sequence();
        });

        // 4. The stack is last-in-first-out, so push in REVERSE: the most urgent ends on top.
        for (std::size_t i = cargo.size(); i > 0; --i) {
            van_.push(cargo[i - 1]);
            cargo[i - 1]->setStatus(Status::InTransit);
        }
        return "{\"count\":" + std::to_string(loaded) + ",\"top\":" + json::quote(cargo.front()->id()) + "}";
    }

    std::string deliver() {
        Parcel* p = nullptr;
        if (!van_.pop(p)) throw std::runtime_error("Van is empty.");
        p->setStatus(Status::Delivered);
        return "{\"id\":" + json::quote(p->id()) + "}";
    }

    std::string cancelStop(const std::string& body) {
        const std::string name = json::getString(body, "name");
        auto it = stops_.find(name);
        if (it == stops_.end()) throw std::runtime_error("That stop can't be cancelled.");
        route_.remove(it->second);                    // O(1) unlink through the stored handle
        stops_.erase(it);
        return "{\"name\":" + json::quote(name) + "}";
    }

    std::string state() const {
        std::string parcels = "[";
        bool first = true;
        for (const auto& p : hub_.parcels()) {
            parcels += (first ? "" : ",") + parcelJson(*p);
            first = false;
        }
        std::vector<std::string> belt, van;
        for (std::size_t i = 0; i < belt_.size(); ++i) belt.push_back(belt_.at(i)->id());
        for (std::size_t i = 0; i < van_.size(); ++i) van.push_back(van_.at(i)->id());
        return "{\"parcels\":" + parcels + "],\"belt\":" + idArray(belt) + ",\"van\":" + idArray(van) +
               ",\"route\":" + idArray(route_.names()) + "}";
    }
};
