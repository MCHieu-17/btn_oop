void ParkingService::commit(ParkingLot candidate) {
    // Chỉ thay đổi dữ liệu đang chạy sau khi lưu thành công.
    repository_.save(candidate);
    lot_ = std::move(candidate);
}
