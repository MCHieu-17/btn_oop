#pragma once

#include "models/DateTime.h"
#include "models/LostTicketInfo.h"
#include "models/Vehicle.h"
#include <optional>
#include <string>

class ParkingSession {
private:
    long long id_;
    std::string cardId_;
    Vehicle vehicle_;
    DateTime entry_;
    std::optional<DateTime> exit_;
    long long paidFee_;
    bool manualFee_;
    std::string reason_;
    std::optional<LostTicketInfo> lostTicket_;

public:
    ParkingSession(long long id, const std::string& cardId, const Vehicle& vehicle,
                   const DateTime& entry, std::optional<DateTime> exit = std::nullopt,
                   long long paidFee = 0, bool manualFee = false,
                   const std::string& reason = "",
                   std::optional<LostTicketInfo> lostTicket = std::nullopt);
    long long getId() const;
    const std::string& getCardId() const;
    const Vehicle& getVehicle() const;
    const DateTime& getEntry() const;
    const std::optional<DateTime>& getExit() const;
    long long getPaidFee() const;
    bool hasManualFee() const;
    const std::string& getReason() const;
    bool hasLostTicket() const;
    const std::optional<LostTicketInfo>& getLostTicket() const;
    bool isActive() const;
    void close(const DateTime& exit, long long fee, bool manual, const std::string& reason);
    void closeLostTicket(const DateTime& exit, long long fee, const LostTicketInfo& info,
                         const std::string& reason = "Mất thẻ");
};
