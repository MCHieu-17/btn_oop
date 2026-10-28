#include "storage/ParkingRepository.h"
#include "exceptions/ParkingException.h"
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
        std::string version;
        int capacity;
        long long nextId;
        if (!(header >> version >> capacity >> nextId) || version != "PARKING_DATA_V1") {
            throw ValidationException("Sai phiên bản hoặc cấu trúc tiêu đề.");
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
            row >> std::ws;
            if (!row.eof() || (manual != 0 && manual != 1)) {
                throw ValidationException("Dòng có dữ liệu dư hoặc cờ phí sai.");
            }
            const auto entry = DateTime::parse(entryText);
            std::optional<DateTime> exit;
            if (exitText != "-") exit = DateTime::parse(exitText);
            ParkingSession session(id, card, Vehicle(plate, static_cast<VehicleType>(type)),
                                   entry, exit, fee, manual == 1, reason);
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
        output << "PARKING_DATA_V1 " << lot.getCapacity() << ' ' << lot.getNextId() << '\n';
        for (const auto& session : lot.getSessions()) {
            output << session.getId() << ' ' << std::quoted(session.getCardId()) << ' '
                   << std::quoted(session.getVehicle().getPlate()) << ' '
                   << static_cast<int>(session.getVehicle().getType()) << ' '
                   << std::quoted(session.getEntry().toString()) << ' '
                   << std::quoted(session.getExit() ? session.getExit()->toString() : "-") << ' '
                   << session.getPaidFee() << ' ' << (session.hasManualFee() ? 1 : 0) << ' '
                   << std::quoted(session.getReason()) << '\n';
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
