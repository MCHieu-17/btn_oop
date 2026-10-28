#include "models/Vehicle.h"
#include "exceptions/ParkingException.h"
#include <cctype>

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string Vehicle::normalizePlate(const std::string& plate) {
    std::string result;
    for (unsigned char c : plate) {
        if (c == '-' || c == '.' || std::isspace(c)) continue;
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z'))) {
            throw ValidationException("Biển số chỉ gồm chữ Latin, số và dấu phân cách.");
        }
        result += static_cast<char>(std::toupper(c));
    }
    if (result.size() < 3 || result.size() > 20) {
        throw ValidationException("Biển số cần từ 3 đến 20 chữ hoặc số.");
    }
    return result;
}

std::string normalizeCardId(const std::string& text) {
    std::string result = trim(text);
    if (result.empty() || result.size() > 64) {
        throw ValidationException("Mã thẻ cần từ 1 đến 64 ký tự.");
    }
    for (char& c : result) {
        const auto value = static_cast<unsigned char>(c);
        if (!((value >= '0' && value <= '9') || (value >= 'A' && value <= 'Z') ||
              (value >= 'a' && value <= 'z') || c == '-' || c == '_')) {
            throw ValidationException("Mã thẻ chỉ gồm chữ Latin, số, dấu - hoặc _.");
        }
        c = static_cast<char>(std::toupper(value));
    }
    return result;
}

Vehicle::Vehicle(const std::string& plate, VehicleType type)
    : plate_(normalizePlate(plate)), type_(type) {
    if (type != VehicleType::Motorbike && type != VehicleType::ElectricMotorbike) {
        throw ValidationException("Loại xe không được hỗ trợ.");
    }
}
const std::string& Vehicle::getPlate() const { return plate_; }
VehicleType Vehicle::getType() const { return type_; }
std::string Vehicle::typeName() const {
    return type_ == VehicleType::Motorbike ? "Xe máy" : "Xe máy điện";
}
