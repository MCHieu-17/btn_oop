#pragma once

#include "models/LostTicketInfo.h"
#include "pricing/FeeCalculator.h"
#include "storage/ParkingRepository.h"
#include <optional>

// Báo giá chỉ để hiển thị; xác nhận trả xe luôn kiểm tra lại lượt và tính lại phí.
struct CheckoutQuote {
    long long sessionId;
    std::string cardId;
    std::string plate;
    DateTime entry;
    DateTime exit;
    std::optional<long long> fee;
    bool manualRequired;
};

struct DailyStatistics {
    int entries = 0;
    int exits = 0;
    long long revenue = 0;
};

class ParkingService {
private:
    ParkingRepository repository_;
    ParkingLot lot_;
    FeeCalculator calculator_;
    void commit(ParkingLot candidate);

public:
    ParkingService(ParkingRepository repository, int initialCapacity);
    const ParkingLot& getLot() const;
    long long checkIn(const std::string& cardId, const Vehicle& vehicle, const DateTime& entry);
    CheckoutQuote prepareCheckout(const std::string& cardId, const DateTime& exit) const;
    void confirmCheckout(const CheckoutQuote& quote,
                         std::optional<long long> manualFee = std::nullopt,
                         const std::string& reason = "");
    CheckoutQuote prepareLostTicketCheckout(const std::string& plate, const DateTime& exit) const;
    void confirmLostTicketCheckout(const CheckoutQuote& quote, long long fee,
                                   const LostTicketInfo& info);
    std::vector<ParkingSession> searchByCard(const std::string& cardId) const;
    std::vector<ParkingSession> searchByPlate(const std::string& plate) const;
    std::vector<ParkingSession> getLostTicketSessions() const;
    DailyStatistics statistics(const std::string& date) const;
    void changeCapacity(int capacity);
};
