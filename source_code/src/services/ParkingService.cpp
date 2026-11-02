#include "services/ParkingService.h"
#include "exceptions/ParkingException.h"
#include <limits>
#include <utility>

ParkingService::ParkingService(ParkingRepository repository, int initialCapacity)
    : repository_(std::move(repository)),
      lot_(repository_.hasData() ? repository_.load() : ParkingLot(initialCapacity)) {
    if (!repository_.hasData()) repository_.save(lot_);
}

const ParkingLot& ParkingService::getLot() const { return lot_; }
void ParkingService::commit(ParkingLot candidate) {
    // Chỉ thay đổi dữ liệu đang chạy sau khi lưu thành công.
    repository_.save(candidate);
    lot_ = std::move(candidate);
}
long long ParkingService::checkIn(const std::string& cardId, const Vehicle& vehicle,
                                 const DateTime& entry) {
    ParkingLot candidate = lot_;
    const long long id = candidate.recordEntry(cardId, vehicle, entry);
    commit(std::move(candidate));
    return id;
}

CheckoutQuote ParkingService::prepareCheckout(const std::string& cardId, const DateTime& exit) const {
    const auto& session = lot_.findActiveByCard(cardId);
    if (exit < session.getEntry()) throw ValidationException("Giờ ra không được trước giờ vào.");
    const bool overnight = !session.getEntry().sameDate(exit);
    std::optional<long long> fee;
    if (!overnight) fee = calculator_.calculate(session.getEntry(), exit);
    return {session.getId(), session.getCardId(), session.getVehicle().getPlate(),
            session.getEntry(), exit, fee, overnight};
}

void ParkingService::confirmCheckout(const CheckoutQuote& quote,
                                     std::optional<long long> manualFee,
                                     const std::string& reason) {
    const auto verified = prepareCheckout(quote.cardId, quote.exit);
    if (verified.sessionId != quote.sessionId || verified.plate != quote.plate ||
        verified.entry != quote.entry) {
        throw ConflictException("Báo giá không còn ứng với lượt gửi hiện tại. Hãy quẹt lại thẻ.");
    }
    long long fee;
    if (verified.manualRequired) {
        if (!manualFee || *manualFee < 0 || trim(reason).empty()) {
            throw ValidationException("Lượt qua ngày cần phí không âm và lý do thu phí.");
        }
        fee = *manualFee;
    } else {
        if (manualFee || !trim(reason).empty()) {
            throw ValidationException("Lượt cùng ngày phải áp dụng phí tự động.");
        }
        fee = *verified.fee;
    }
    ParkingLot candidate = lot_;
    candidate.recordExit(verified.sessionId, verified.exit, fee, verified.manualRequired, reason);
    commit(std::move(candidate));
}

CheckoutQuote ParkingService::prepareLostTicketCheckout(const std::string& plate,
                                                       const DateTime& exit) const {
    const auto& session = lot_.findActiveByPlate(plate);
    if (exit < session.getEntry()) throw ValidationException("Giờ ra không được trước giờ vào.");
    std::optional<long long> fee;
    if (session.getEntry().sameDate(exit)) fee = calculator_.calculate(session.getEntry(), exit);
    return {session.getId(), session.getCardId(), session.getVehicle().getPlate(),
            session.getEntry(), exit, fee, true};
}

void ParkingService::confirmLostTicketCheckout(const CheckoutQuote& quote, long long fee,
                                               const LostTicketInfo& info) {
    const auto verified = prepareLostTicketCheckout(quote.plate, quote.exit);
    if (verified.sessionId != quote.sessionId || verified.plate != quote.plate ||
        verified.entry != quote.entry) {
        throw ConflictException("Báo giá không còn ứng với lượt gửi hiện tại. Hãy tra cứu lại biển số.");
    }
    if (fee < 0) throw ValidationException("Phí không được âm.");
    ParkingLot candidate = lot_;
    candidate.recordLostTicketExit(verified.sessionId, verified.exit, fee, info);
    commit(std::move(candidate));
}

std::vector<ParkingSession> ParkingService::searchByCard(const std::string& cardId) const {
    const auto normalized = normalizeCardId(cardId);
    std::vector<ParkingSession> result;
    for (const auto& session : lot_.getSessions()) {
        if (session.getCardId() == normalized) result.push_back(session);
    }
    return result;
}
std::vector<ParkingSession> ParkingService::searchByPlate(const std::string& plate) const {
    const auto normalized = Vehicle::normalizePlate(plate);
    std::vector<ParkingSession> result;
    for (const auto& session : lot_.getSessions()) {
        if (session.getVehicle().getPlate() == normalized) result.push_back(session);
    }
    return result;
}
std::vector<ParkingSession> ParkingService::getLostTicketSessions() const {
    std::vector<ParkingSession> result;
    for (const auto& session : lot_.getSessions()) {
        if (session.hasLostTicket()) result.push_back(session);
    }
    return result;
}
DailyStatistics ParkingService::statistics(const std::string& date) const {
    const auto normalized = DateTime::parse(date + " 00:00").dateString();
    DailyStatistics result;
    for (const auto& session : lot_.getSessions()) {
        if (session.getEntry().dateString() == normalized) ++result.entries;
        if (session.getExit() && session.getExit()->dateString() == normalized) {
            ++result.exits;
            if (session.getPaidFee() > std::numeric_limits<long long>::max() - result.revenue) {
                throw ValidationException("Tổng doanh thu vượt miền giá trị số nguyên.");
            }
            result.revenue += session.getPaidFee();
        }
    }
    return result;
}
void ParkingService::changeCapacity(int capacity) {
    ParkingLot candidate = lot_;
    candidate.changeCapacity(capacity);
    commit(std::move(candidate));
}
