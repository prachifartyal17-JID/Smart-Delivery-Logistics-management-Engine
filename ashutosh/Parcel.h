// Parcel.h  (Ashutosh) - shared Parcel class + Strategy pattern for shipping cost
#pragma once
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

// ---------- Strategy pattern: interchangeable shipping-cost rules ----------
class ShippingStrategy {
public:
    virtual ~ShippingStrategy() = default;
    virtual double calcCost(double weightKg, double distanceKm) const = 0;
    virtual std::string name() const = 0;
};

class StandardRate : public ShippingStrategy {
public:
    double calcCost(double weightKg, double distanceKm) const override {
        return 40.0 + weightKg * 10.0 + distanceKm * 1.5;
    }
    std::string name() const override { return "Standard Ground"; }
};

class ExpressSurgeRate : public ShippingStrategy {
public:
    double calcCost(double weightKg, double distanceKm) const override {
        return 100.0 + weightKg * 22.0 + distanceKm * 3.5;
    }
    std::string name() const override { return "Prime Express"; }
};

// Simple lifecycle for now; the State pattern (Swastika, 40% phase) will replace this enum.
enum class Status { Placed, Sorted, InTransit, Delivered };

// ---------- The one Parcel class every module uses ----------
class Parcel {
public:
    Parcel(std::string id, std::string destination, std::string pincode,
           double weightKg, double distanceKm, int priority,
           std::shared_ptr<const ShippingStrategy> strategy)
        : id_(std::move(id)),
          destination_(std::move(destination)),
          pincode_(std::move(pincode)),
          weightKg_(weightKg),
          distanceKm_(distanceKm),
          priority_(priority),
          strategy_(std::move(strategy)),
          sequence_(nextSequence_++) {
        if (id_.empty())          throw std::invalid_argument("Parcel ID cannot be empty");
        if (weightKg_ <= 0.0)     throw std::invalid_argument("Weight must be positive");
        if (distanceKm_ <= 0.0)   throw std::invalid_argument("Distance must be positive");
        if (priority_ < 1)        throw std::invalid_argument("Priority must be 1 or higher");
        if (!strategy_)           throw std::invalid_argument("Shipping strategy required");
    }

    // A parcel is a unique physical object, so it must never be copied.
    Parcel(const Parcel&) = delete;
    Parcel& operator=(const Parcel&) = delete;

    const std::string& id() const          { return id_; }
    const std::string& destination() const { return destination_; }
    const std::string& pincode() const     { return pincode_; }
    double weightKg() const                { return weightKg_; }
    double distanceKm() const              { return distanceKm_; }
    int priority() const                   { return priority_; }       // 1 = most urgent
    std::size_t sequence() const           { return sequence_; }       // arrival order (heap tie-breaker)
    std::string service() const            { return strategy_->name(); }
    double cost() const                    { return strategy_->calcCost(weightKg_, distanceKm_); }
    Status status() const                  { return status_; }
    void setStatus(Status s)               { status_ = s; }
    std::string statusText() const {
        switch (status_) {
            case Status::Placed:    return "Placed";
            case Status::Sorted:    return "Sorted";
            case Status::InTransit: return "In transit";
            case Status::Delivered: return "Delivered";
        }
        return "Unknown";
    }

    void print(std::ostream& os = std::cout) const {
        os << "Parcel " << id_ << " | " << destination_ << " (" << pincode_ << ")"
           << " | Priority " << priority_ << " | " << service()
           << " | Rs " << cost() << '\n';
    }

private:
    std::string id_;
    std::string destination_;
    std::string pincode_;
    double weightKg_;
    double distanceKm_;
    int priority_;
    std::shared_ptr<const ShippingStrategy> strategy_;
    std::size_t sequence_;
    Status status_ = Status::Placed;

    static inline std::size_t nextSequence_ = 0;
};
