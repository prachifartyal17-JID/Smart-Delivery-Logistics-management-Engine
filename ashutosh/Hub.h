// Hub.h  (Ashutosh) - Hub OWNS every parcel (RAII). Other modules hold plain Parcel* (non-owning).
#pragma once
#include <memory>
#include <vector>
#include "Parcel.h"

class Hub {
public:
    explicit Hub(std::string name) : name_(std::move(name)) {}

    // Takes ownership; returns a non-owning pointer for the DSA modules to store.
    Parcel* addParcel(std::unique_ptr<Parcel> parcel) {
        if (!parcel) return nullptr;
        parcels_.push_back(std::move(parcel));
        return parcels_.back().get();
    }

    std::size_t count() const { return parcels_.size(); }
    const std::vector<std::unique_ptr<Parcel>>& parcels() const { return parcels_; }
    const std::string& name() const { return name_; }

    void showManifest(std::ostream& os = std::cout) const {
        os << "Package list for " << name_ << ":\n";
        for (const auto& p : parcels_) p->print(os);
    }

private:
    std::string name_;
    std::vector<std::unique_ptr<Parcel>> parcels_;
};
