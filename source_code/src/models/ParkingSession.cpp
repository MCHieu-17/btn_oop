#include "models/ParkingSession.h"
#include "exceptions/ParkingException.h"
#include <utility>

ParkingSession::ParkingSession(long long id, const std::string& cardId, const Vehicle& vehicle,
                               const DateTime& entry, std::optional<DateTime> exit,
                               long long paidFee, bool manualFee, const std::string& reason,
                               std::optional<LostTicketInfo> lostTicket)
    : id_(id), cardId_(normalizeCardId(cardId)), vehicle_(vehicle), entry_(entry),
      paidFee_(0), manualFee_(false), lostTicket_(std::move(lostTicket)) {
    if (id <= 0) throw ValidationException("Mã lượt gửi phải dương.");
    if (exit) {
        if (lostTicket_) {
            if (!manualFee) throw ValidationException("Lượt mất thẻ phải có phí thủ công.");
            closeLostTicket(*exit, paidFee, *lostTicket_, reason);
        } else {
            close(*exit, paidFee, manualFee, reason);
        }
    } else {
        if (lostTicket_) throw ValidationException("Lượt đang gửi không thể có thông tin mất thẻ.");
        if (paidFee != 0 || manualFee || !reason.empty()) {
            throw ValidationException("Lượt đang gửi không được chứa thông tin thu tiền.");
        }
    }
}

long long ParkingSession::getId() const { return id_; }
const std::string& ParkingSession::getCardId() const { return cardId_; }
const Vehicle& ParkingSession::getVehicle() const { return vehicle_; }
const DateTime& ParkingSession::getEntry() const { return entry_; }
const std::optional<DateTime>& ParkingSession::getExit() const { return exit_; }
long long ParkingSession::getPaidFee() const { return paidFee_; }
bool ParkingSession::hasManualFee() const { return manualFee_; }
const std::string& ParkingSession::getReason() const { return reason_; }
bool ParkingSession::hasLostTicket() const { return lostTicket_.has_value(); }
const std::optional<LostTicketInfo>& ParkingSession::getLostTicket() const { return lostTicket_; }
bool ParkingSession::isActive() const { return !exit_.has_value(); }

void ParkingSession::close(const DateTime& exit, long long fee, bool manual,
                           const std::string& reason) {
    if (!isActive()) throw ConflictException("Lượt gửi đã kết thúc.");
    if (exit < entry_) throw ValidationException("Giờ ra không được trước giờ vào.");
    if (fee < 0) throw ValidationException("Phí không được âm.");
    const bool overnight = !entry_.sameDate(exit);
    if (manual != overnight) {
        throw ValidationException("Chỉ lượt qua ngày mới được nhập phí thủ công.");
    }
    const std::string normalizedReason = trim(reason);
    if ((manual && normalizedReason.empty()) || (!manual && !normalizedReason.empty()) ||
        normalizedReason.size() > 500 || normalizedReason.find_first_of("\r\n") != std::string::npos) {
        throw ValidationException("Lý do phí thủ công cần từ 1 đến 500 ký tự trên một dòng.");
    }
    exit_ = exit;
    paidFee_ = fee;
    manualFee_ = manual;
    reason_ = normalizedReason;
}

void ParkingSession::closeLostTicket(const DateTime& exit, long long fee,
                                     const LostTicketInfo& info, const std::string& reason) {
    if (!isActive()) throw ConflictException("Lượt gửi đã kết thúc.");
    if (exit < entry_) throw ValidationException("Giờ ra không được trước giờ vào.");
    if (fee < 0) throw ValidationException("Phí không được âm.");
    const std::string normalizedReason = trim(reason).empty() ? "Mất thẻ" : trim(reason);
    if (normalizedReason.size() > 500 ||
        normalizedReason.find_first_of("\r\n") != std::string::npos) {
        throw ValidationException("Lý do mất thẻ cần từ 1 đến 500 ký tự trên một dòng.");
    }
    exit_ = exit;
    paidFee_ = fee;
    manualFee_ = true;
    reason_ = normalizedReason;
    lostTicket_ = info;
}
