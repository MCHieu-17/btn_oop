#include "storage/ParkingRepository.h"
#include "exceptions/ParkingException.h"
#include "models/LostTicketInfo.h"
#include "pricing/FeeCalculator.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {
bool existsChecked(const std::filesystem::path& path) {
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) throw StorageException("Không kiểm tra được tệp: " + error.message());
    return exists;
}
std::filesystem::path backupPath(const std::filesystem::path& path) {
    auto result = path;
    result += ".bak";
    return result;
}
}

ParkingRepository::ParkingRepository(std::filesystem::path path) : path_(std::move(path)) {
    if (path_.empty()) throw StorageException("Đường dẫn dữ liệu không được rỗng.");
}
const std::filesystem::path& ParkingRepository::getPath() const { return path_; }
bool ParkingRepository::hasData() const {
    return existsChecked(path_) || existsChecked(backupPath(path_));
}

ParkingLot ParkingRepository::load() const {
    // Nếu quá trình trước dừng giữa hai lần đổi tên, bản sao lưu vẫn chứa dữ liệu cũ.
    const auto source = existsChecked(path_) ? path_ : backupPath(path_);
    std::ifstream input(source, std::ios::binary);
    if (!input) throw StorageException("Không mở được tệp dữ liệu để đọc.");
    std::string line;
    int lineNumber = 1;
    try {
        if (!std::getline(input, line)) throw ValidationException("Thiếu dòng tiêu đề.");
        std::istringstream header(line);
        std::string tag;
        int capacity;
        long long nextId;
        if (!(header >> tag >> capacity >> nextId) || tag != "PARKING_DATA") {
            throw ValidationException("Sai cấu trúc tiêu đề.");
        }
        header >> std::ws;
        if (!header.eof()) throw ValidationException("Tiêu đề có dữ liệu dư.");
        std::vector<ParkingSession> sessions;
        FeeCalculator calculator;
        while (std::getline(input, line)) {
            ++lineNumber;
            if (trim(line).empty()) continue;
            if (line.size() > 4096) throw ValidationException("Dòng dữ liệu quá dài.");
            std::istringstream row(line);
            long long id, fee;
            int type, manual;
            std::string card, plate, entryText, exitText, reason;
            if (!(row >> id >> std::quoted(card) >> std::quoted(plate) >> type
                      >> std::quoted(entryText) >> std::quoted(exitText) >> fee
                      >> manual >> std::quoted(reason))) {
                throw ValidationException("Thiếu trường hoặc trường có kiểu dữ liệu sai.");
            }
            int lostFlag = 0;
            std::string studentId, studentName, ownerName, ownerId, className, phone, note;
            if (!(row >> lostFlag >> std::quoted(studentId) >> std::quoted(studentName)
                      >> std::quoted(ownerName) >> std::quoted(ownerId)
                      >> std::quoted(className) >> std::quoted(phone) >> std::quoted(note))) {
                throw ValidationException("Thiếu trường thông tin mất thẻ.");
            }
            row >> std::ws;
            if (!row.eof() || (manual != 0 && manual != 1) ||
                (lostFlag != 0 && lostFlag != 1)) {
                throw ValidationException("Dòng có dữ liệu dư hoặc cờ phí/mất thẻ sai.");
            }
            const auto entry = DateTime::parse(entryText);
            std::optional<DateTime> exit;
            if (exitText != "-") exit = DateTime::parse(exitText);
            std::optional<LostTicketInfo> lostInfo;
            if (lostFlag == 1) {
                if (!exit) throw ValidationException("Lượt đang gửi không thể có thông tin mất thẻ.");
                if (manual != 1) throw ValidationException("Lượt mất thẻ phải có phí thủ công.");
                lostInfo = LostTicketInfo(studentId, studentName, ownerName, ownerId,
                                          className, phone, note);
            } else if (!studentId.empty() || !studentName.empty() || !ownerName.empty() ||
                       !ownerId.empty() || !className.empty() || !phone.empty() ||
                       !note.empty()) {
                throw ValidationException("Lượt không mất thẻ không được có thông tin mất thẻ.");
            }
            ParkingSession session(id, card, Vehicle(plate, static_cast<VehicleType>(type)),
                                   entry, exit, fee, manual == 1, reason, lostInfo);
            if (exit && !session.hasManualFee() && calculator.calculate(entry, *exit) != fee) {
                throw ValidationException("Phí cùng ngày không khớp biểu phí.");
            }
            sessions.push_back(std::move(session));
        }
        if (input.bad()) throw ValidationException("Lỗi đọc nội dung tệp.");
        return ParkingLot(capacity, nextId, std::move(sessions));
    } catch (const ParkingException& error) {
        throw StorageException("Dữ liệu không hợp lệ gần dòng " + std::to_string(lineNumber) +
                               ": " + error.what() + " Không ghi đè dữ liệu cũ.");
    }
}

void ParkingRepository::save(const ParkingLot& lot) const {
    auto temporary = path_;
    temporary += ".tmp";
    const auto backup = backupPath(path_);
    try {
        if (!path_.parent_path().empty()) std::filesystem::create_directories(path_.parent_path());
        if (existsChecked(path_) && !std::filesystem::is_regular_file(path_)) {
            throw StorageException("Đường dẫn dữ liệu không phải tệp thường.");
        }
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) throw StorageException("Không tạo được tệp tạm để lưu dữ liệu.");
        output << "PARKING_DATA " << lot.getCapacity() << ' ' << lot.getNextId() << '\n';
        for (const auto& session : lot.getSessions()) {
            output << session.getId() << ' ' << std::quoted(session.getCardId()) << ' '
                   << std::quoted(session.getVehicle().getPlate()) << ' '
                   << static_cast<int>(session.getVehicle().getType()) << ' '
                   << std::quoted(session.getEntry().toString()) << ' '
                   << std::quoted(session.getExit() ? session.getExit()->toString() : "-") << ' '
                   << session.getPaidFee() << ' ' << (session.hasManualFee() ? 1 : 0) << ' '
                   << std::quoted(session.getReason());
            if (session.hasLostTicket()) {
                const auto& info = *session.getLostTicket();
                output << " 1 " << std::quoted(info.getStudentId()) << ' '
                       << std::quoted(info.getStudentName()) << ' '
                       << std::quoted(info.getOwnerName()) << ' '
                       << std::quoted(info.getOwnerId()) << ' '
                       << std::quoted(info.getClassName()) << ' '
                       << std::quoted(info.getPhone()) << ' '
                       << std::quoted(info.getNote());
            } else {
                output << " 0 \"\" \"\" \"\" \"\" \"\" \"\" \"\"";
            }
            output << '\n';
        }
        output.flush();
        if (!output) throw StorageException("Không ghi đủ dữ liệu vào tệp tạm.");
        output.close();
        if (!output) throw StorageException("Không đóng được tệp tạm sau khi ghi.");

        const bool hadOriginal = existsChecked(path_);
        if (hadOriginal) {
            if (existsChecked(backup)) {
                if (!std::filesystem::is_regular_file(backup)) {
                    throw StorageException("Đường dẫn sao lưu không phải tệp thường.");
                }
                std::filesystem::remove(backup);
            }
            std::filesystem::rename(path_, backup);
        }
        std::error_code replaceError;
        std::filesystem::rename(temporary, path_, replaceError);
        if (replaceError) {
            if (hadOriginal) {
                std::error_code rollbackError;
                std::filesystem::rename(backup, path_, rollbackError);
                if (rollbackError) {
                    throw StorageException("Lưu thất bại; bản dữ liệu cũ còn trong tệp .bak.");
                }
            }
            throw StorageException("Không thay được tệp dữ liệu: " + replaceError.message());
        }
        // Dọn bản sao sau khi tệp chính đã hoàn tất; lỗi dọn dẹp không làm đảo kết quả lưu.
        std::error_code cleanupError;
        std::filesystem::remove(backup, cleanupError);
    } catch (const std::filesystem::filesystem_error& error) {
        throw StorageException("Không lưu được dữ liệu: " + std::string(error.what()));
    }
}
