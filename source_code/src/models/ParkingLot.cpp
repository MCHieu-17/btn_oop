#include "models/ParkingLot.h"
#include "exceptions/ParkingException.h"
#include <limits>
#include <set>
#include <utility>

ParkingLot::ParkingLot(int capacity) : capacity_(capacity), nextId_(1) { validate(); }
ParkingLot::ParkingLot(int capacity, long long nextId, std::vector<ParkingSession> sessions)
    : capacity_(capacity), nextId_(nextId), sessions_(std::move(sessions)) { validate(); }

void ParkingLot::validate() const {
    if (capacity_ < 1 || capacity_ > 1000000) {
        throw CapacityException("Sức chứa phải từ 1 đến 1.000.000 xe.");
    }
    if (nextId_ <= 0) throw ValidationException("Mã lượt kế tiếp không hợp lệ.");
    std::set<long long> ids;
    std::set<std::string> cards, plates;
    for (const auto& session : sessions_) {
        if (!ids.insert(session.getId()).second || session.getId() >= nextId_) {
            throw ConflictException("Dữ liệu có mã lượt trùng hoặc mã lượt kế tiếp sai.");
        }
        if (session.isActive()) {
            if (!cards.insert(session.getCardId()).second ||
                !plates.insert(session.getVehicle().getPlate()).second) {
                throw ConflictException("Dữ liệu có thẻ hoặc biển số đang gửi bị trùng.");
            }
        }
        if (session.hasLostTicket()) {
            if (session.isActive()) {
                throw ConflictException("Lượt đang gửi không thể có thông tin mất thẻ.");
            }
            if (!session.hasManualFee()) {
                throw ConflictException("Lượt mất thẻ phải có phí thủ công.");
            }
        }
    }
    if (cards.size() > static_cast<std::size_t>(capacity_)) {
        throw CapacityException("Số xe đang gửi vượt sức chứa.");
    }
}

int ParkingLot::getCapacity() const { return capacity_; }
long long ParkingLot::getNextId() const { return nextId_; }
const std::vector<ParkingSession>& ParkingLot::getSessions() const { return sessions_; }
std::size_t ParkingLot::activeCount() const {
    std::size_t count = 0;
    for (const auto& session : sessions_) if (session.isActive()) ++count;
    return count;
}
const ParkingSession& ParkingLot::findActiveByCard(const std::string& cardId) const {
    const auto normalized = normalizeCardId(cardId);
    for (const auto& session : sessions_) {
        if (session.isActive() && session.getCardId() == normalized) return session;
    }
    throw NotFoundException("Không có lượt đang gửi ứng với thẻ này.");
}
const ParkingSession& ParkingLot::findActiveByPlate(const std::string& plate) const {
    const auto normalized = Vehicle::normalizePlate(plate);
    for (const auto& session : sessions_) {
        if (session.isActive() && session.getVehicle().getPlate() == normalized) return session;
    }
    throw NotFoundException("Không có lượt đang gửi ứng với biển số này.");
}
long long ParkingLot::recordEntry(const std::string& cardId, const Vehicle& vehicle,
                                 const DateTime& entry) {
    const auto normalized = normalizeCardId(cardId);
    for (const auto& session : sessions_) {
        if (!session.isActive()) continue;
        if (session.getCardId() == normalized) throw ConflictException("Thẻ đang được sử dụng.");
        if (session.getVehicle().getPlate() == vehicle.getPlate()) {
            throw ConflictException("Biển số đã có xe đang gửi trong bãi.");
        }
    }
    if (activeCount() >= static_cast<std::size_t>(capacity_)) throw CapacityException("Bãi đã đầy.");
    if (nextId_ == std::numeric_limits<long long>::max()) {
        throw ValidationException("Đã hết miền giá trị mã lượt gửi.");
    }
    sessions_.emplace_back(nextId_, normalized, vehicle, entry);
    return nextId_++;
}
void ParkingLot::recordExit(long long id, const DateTime& exit, long long fee,
                           bool manual, const std::string& reason) {
    for (auto& session : sessions_) {
        if (session.getId() == id) {
            session.close(exit, fee, manual, reason);
            return;
        }
    }
    throw NotFoundException("Không tìm thấy mã lượt gửi.");
}
void ParkingLot::recordLostTicketExit(long long id, const DateTime& exit, long long fee,
                                      const LostTicketInfo& info, const std::string& reason) {
    for (auto& session : sessions_) {
        if (session.getId() == id) {
            session.closeLostTicket(exit, fee, info, reason);
            return;
        }
    }
    throw NotFoundException("Không tìm thấy mã lượt gửi.");
}
void ParkingLot::changeCapacity(int capacity) {
    if (capacity < 1 || capacity > 1000000 ||
        static_cast<std::size_t>(capacity) < activeCount()) {
        throw CapacityException("Sức chứa cần từ 1 đến 1.000.000 và không nhỏ hơn số xe đang gửi.");
    }
    capacity_ = capacity;
}
