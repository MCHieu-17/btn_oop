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
