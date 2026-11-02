#include "ui/ConsoleUI.h"
#include "exceptions/ParkingException.h"
#include <algorithm>
#include <iostream>
#include <limits>

ConsoleUI::ConsoleUI(ParkingService& service, bool demo) : service_(service), demo_(demo) {}

std::string ConsoleUI::readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string text;
    if (!std::getline(std::cin, text)) throw ValidationException("Luồng nhập đã kết thúc.");
    return trim(text);
}
long long ConsoleUI::readNumber(const std::string& prompt, long long min, long long max) {
    const auto text = readLine(prompt);
    try {
        std::size_t position = 0;
        const auto value = std::stoll(text, &position);
        if (position != text.size() || value < min || value > max) throw std::out_of_range("range");
        return value;
    } catch (const std::exception&) {
        throw ValidationException("Cần nhập số nguyên từ " + std::to_string(min) + " đến " +
                                   std::to_string(max) + ".");
    }
}
bool ConsoleUI::confirm(const std::string& prompt) {
    const auto value = readLine(prompt + " (y/N): ");
    return value == "y" || value == "Y";
}
std::string ConsoleUI::money(long long amount) {
    std::string result = std::to_string(amount);
    for (int position = static_cast<int>(result.size()) - 3; position > 0; position -= 3) {
        result.insert(static_cast<std::size_t>(position), ".");
    }
    return result + " đồng";
}
DateTime ConsoleUI::readTime(const std::string& label) const {
    if (!demo_) return DateTime::now();
    return DateTime::parse(readLine(label + " [YYYY-MM-DD HH:MM]: "));
}

void ConsoleUI::receiveVehicle() {
    const auto card = readLine("Quẹt/nhập mã thẻ: ");
    const auto plate = readLine("Biển số xe: ");
    const auto type = readNumber("Loại xe (1. Xe máy / 2. Xe máy điện): ", 1, 2);
    const auto time = readTime("Giờ vào");
    const auto id = service_.checkIn(card, Vehicle(plate, static_cast<VehicleType>(type)), time);
    std::cout << "ĐÃ NHẬN XE - lượt #" << id << ", thẻ " << normalizeCardId(card)
              << ", biển số " << Vehicle::normalizePlate(plate) << ", vào " << time.toString()
              << ".\nDữ liệu đã lưu. Giao thẻ cho sinh viên.\n";
}

void ConsoleUI::releaseVehicle() {
    const auto card = readLine("Quẹt/nhập mã thẻ trả xe: ");
    const auto time = readTime("Giờ ra");
    const auto quote = service_.prepareCheckout(card, time);
    const auto& session = service_.getLot().findActiveByCard(card);
    std::cout << "Lượt #" << quote.sessionId << " | Thẻ: " << quote.cardId
              << " | Biển số: " << quote.plate << " | " << session.getVehicle().typeName()
              << "\nGiờ vào: " << quote.entry.toString() << " | Giờ ra: " << quote.exit.toString() << '\n';
    if (!confirm("Biển số trên màn hình có khớp xe thực tế?")) {
        std::cout << "Đã hủy trả xe. Lượt gửi vẫn đang hoạt động.\n";
        return;
    }
    std::optional<long long> manualFee;
    std::string reason;
    if (quote.manualRequired) {
        std::cout << "Xe gửi qua ngày: cần nhập phí theo xử lý thực tế tại bãi.\n";
        manualFee = readNumber("Phí được thu (VND, số nguyên): ", 0,
                               std::numeric_limits<long long>::max());
        reason = readLine("Lý do/căn cứ thu phí: ");
        if (reason.empty() || reason.size() > 500) {
            throw ValidationException("Lý do cần từ 1 đến 500 ký tự.");
        }
    }
    std::cout << "SỐ TIỀN CẦN THU: " << money(manualFee ? *manualFee : *quote.fee) << '\n';
    if (!confirm("Đã thu đủ tiền và xác nhận cho xe ra?")) {
        std::cout << "Chưa đóng lượt gửi; xe vẫn nằm trong danh sách đang gửi.\n";
        return;
    }
    service_.confirmCheckout(quote, manualFee, reason);
    std::cout << "ĐÃ TRẢ XE. Dữ liệu đã lưu, thẻ " << quote.cardId << " có thể dùng lại.\n";
}

void ConsoleUI::lostTicket() {
    const auto plate = readLine("Biển số xe bị mất thẻ: ");
    const auto time = readTime("Giờ ra/giờ báo mất");
    const auto quote = service_.prepareLostTicketCheckout(plate, time);
    const auto& session = service_.getLot().findActiveByPlate(plate);
    std::cout << "Lượt #" << quote.sessionId << " | Thẻ: " << quote.cardId
              << " | Biển số: " << quote.plate << " | " << session.getVehicle().typeName()
              << "\nGiờ vào: " << quote.entry.toString() << " | Giờ ra: " << quote.exit.toString()
              << '\n';
    if (quote.fee) {
        std::cout << "Phí theo biểu phí (tham khảo): " << money(*quote.fee) << '\n';
    } else {
        std::cout << "Lượt qua ngày: không có phí tự động, cần nhập phí thủ công.\n";
    }
    const auto studentId = readLine("Mã SV/CCCD sinh viên: ");
    const auto studentName = readLine("Họ tên sinh viên: ");
    const auto ownerName = readLine("Họ tên chủ xe trên giấy tờ: ");
    const auto ownerId = readLine("CCCD/CMND chủ xe (có thể bỏ trống): ");
    const auto className = readLine("Lớp (có thể bỏ trống): ");
    const auto phone = readLine("Số điện thoại (có thể bỏ trống): ");
    const auto note = readLine("Ghi chú (có thể bỏ trống): ");
    const auto fee = readNumber("Phí thu thủ công (VND, số nguyên): ", 0,
                                std::numeric_limits<long long>::max());
    const LostTicketInfo info(studentId, studentName, ownerName, ownerId, className, phone, note);
    std::cout << "XÁC NHẬN MẤT THẺ\n"
              << "Sinh viên: " << info.getStudentId() << " - " << info.getStudentName() << '\n'
              << "Chủ xe: " << info.getOwnerName() << '\n'
              << "Phí thu: " << money(fee) << '\n';
    if (!confirm("Lưu thông tin mất thẻ và cho xe ra?")) {
        std::cout << "Đã hủy xử lý mất thẻ. Lượt gửi vẫn đang hoạt động.\n";
        return;
    }
    service_.confirmLostTicketCheckout(quote, fee, info);
    std::cout << "ĐÃ XỬ LÝ MẤT THẺ. Lượt #" << quote.sessionId
              << " đã đóng, dữ liệu đã lưu.\n";
}

void ConsoleUI::printSession(const ParkingSession& session) {
    std::cout << '#' << session.getId() << " | Thẻ " << session.getCardId() << " | "
              << session.getVehicle().getPlate() << " | " << session.getVehicle().typeName()
              << " | Vào " << session.getEntry().toString();
    if (session.isActive()) {
        std::cout << " | ĐANG GỬI\n";
    } else {
        std::cout << " | Ra " << session.getExit()->toString() << " | "
                  << money(session.getPaidFee()) << '\n';
        if (session.hasManualFee()) std::cout << "   Phí thủ công: " << session.getReason() << '\n';
        if (session.hasLostTicket()) {
            const auto& info = *session.getLostTicket();
            std::cout << "   MẤT THẺ | SV: " << info.getStudentId() << " - "
                      << info.getStudentName();
            if (!info.getClassName().empty()) std::cout << " | Lớp: " << info.getClassName();
            if (!info.getPhone().empty()) std::cout << " | SĐT: " << info.getPhone();
            std::cout << "\n   Chủ xe: " << info.getOwnerName();
            if (!info.getOwnerId().empty()) std::cout << " | CCCD chủ xe: " << info.getOwnerId();
            if (!info.getNote().empty()) std::cout << " | Ghi chú: " << info.getNote();
            std::cout << '\n';
        }
    }
}
void ConsoleUI::showActive() const {
    const auto& lot = service_.getLot();
    const auto active = lot.activeCount();
    std::cout << "ĐANG GỬI: " << active << '/' << lot.getCapacity()
              << " | CÒN TRỐNG: " << static_cast<std::size_t>(lot.getCapacity()) - active << '\n';
    for (const auto& session : lot.getSessions()) if (session.isActive()) printSession(session);
    if (active == 0) std::cout << "Chưa có xe đang gửi.\n";
}
void ConsoleUI::search() const {
    const auto mode = readNumber("Tìm theo (1. Mã thẻ / 2. Biển số): ", 1, 2);
    const auto query = readLine("Giá trị cần tìm: ");
    const auto result = mode == 1 ? service_.searchByCard(query) : service_.searchByPlate(query);
    std::cout << "Tìm thấy " << result.size() << " lượt (gồm cả lịch sử).\n";
    for (const auto& session : result) printSession(session);
}
void ConsoleUI::showHistory() const {
    const auto& sessions = service_.getLot().getSessions();
    std::cout << "TOÀN BỘ LỊCH SỬ: " << sessions.size() << " lượt.\n";
    for (const auto& session : sessions) printSession(session);
}
void ConsoleUI::showLostTickets() const {
    const auto sessions = service_.getLostTicketSessions();
    std::cout << "DANH SÁCH MẤT THẺ: " << sessions.size() << " lượt.\n";
    for (const auto& session : sessions) printSession(session);
    if (sessions.empty()) std::cout << "Chưa có lượt mất thẻ nào.\n";
}
void ConsoleUI::showStatistics() const {
    const auto date = readLine("Ngày thống kê [YYYY-MM-DD]: ");
    const auto result = service_.statistics(date);
    std::cout << "Ngày " << date << " | Lượt vào: " << result.entries << " | Lượt ra: "
              << result.exits << " | Doanh thu thực thu: " << money(result.revenue) << '\n';
}
void ConsoleUI::changeCapacity() {
    const auto capacity = readNumber("Sức chứa mới: ", 1, 1000000);
    service_.changeCapacity(static_cast<int>(capacity));
    std::cout << "Đã lưu sức chứa mới.\n";
}

void ConsoleUI::run() {
    std::cout << "\nHỆ THỐNG QUẢN LÝ BÃI ĐỖ XE SINH VIÊN\n"
              << (demo_ ? "CHẾ ĐỘ DEMO - nhập thời gian, dữ liệu riêng.\n"
                        : "CHẾ ĐỘ THƯỜNG - sử dụng giờ hệ thống.\n");
    while (true) {
        std::cout << "\n1. Nhận xe          2. Trả xe            3. Xe đang gửi\n"
                     "4. Tìm kiếm         5. Lịch sử          6. Thống kê ngày\n"
                     "7. Đổi sức chứa     8. Xử lý mất thẻ    9. Danh sách mất thẻ\n"
                     "0. Thoát\n";
        try {
            const auto choice = readNumber("Chọn chức năng: ", 0, 9);
            switch (choice) {
                case 0: std::cout << "Đã thoát. Mọi giao dịch thành công đã được lưu.\n"; return;
                case 1: receiveVehicle(); break;
                case 2: releaseVehicle(); break;
                case 3: showActive(); break;
                case 4: search(); break;
                case 5: showHistory(); break;
                case 6: showStatistics(); break;
                case 7: changeCapacity(); break;
                case 8: lostTicket(); break;
                case 9: showLostTickets(); break;
            }
        } catch (const ParkingException& error) {
            if (std::cin.eof()) {
                std::cout << "\nKết thúc nhập. Các giao dịch thành công đã được lưu.\n";
                return;
            }
            std::cout << "Lỗi: " << error.what() << '\n';
        }
    }
}
