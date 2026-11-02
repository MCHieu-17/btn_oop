#include "models/LostTicketInfo.h"
#include "exceptions/ParkingException.h"
#include "models/Vehicle.h"

namespace {
std::string requiredText(const std::string& value, const std::string& label,
                         std::size_t maxLength) {
    const auto normalized = trim(value);
    if (normalized.empty()) throw ValidationException(label + " không được để trống.");
    if (normalized.size() > maxLength) {
        throw ValidationException(label + " tối đa " + std::to_string(maxLength) + " byte UTF-8.");
    }
    if (normalized.find_first_of("\r\n") != std::string::npos) {
        throw ValidationException(label + " phải nằm trên một dòng.");
    }
    return normalized;
}

std::string optionalText(const std::string& value, const std::string& label,
                         std::size_t maxLength) {
    const auto normalized = trim(value);
    if (normalized.size() > maxLength) {
        throw ValidationException(label + " tối đa " + std::to_string(maxLength) + " byte UTF-8.");
    }
    if (normalized.find_first_of("\r\n") != std::string::npos) {
        throw ValidationException(label + " phải nằm trên một dòng.");
    }
    return normalized;
}
}

LostTicketInfo::LostTicketInfo(const std::string& studentId, const std::string& studentName,
                               const std::string& ownerName, const std::string& ownerId,
                               const std::string& className, const std::string& phone,
                               const std::string& note)
    : studentId_(requiredText(studentId, "Mã SV/CCCD sinh viên", 50)),
      studentName_(requiredText(studentName, "Họ tên sinh viên", 100)),
      ownerName_(requiredText(ownerName, "Họ tên chủ xe", 100)),
      ownerId_(optionalText(ownerId, "CCCD/CMND chủ xe", 50)),
      className_(optionalText(className, "Lớp", 50)),
      phone_(optionalText(phone, "Số điện thoại", 50)),
      note_(optionalText(note, "Ghi chú", 500)) {}

const std::string& LostTicketInfo::getStudentId() const { return studentId_; }
const std::string& LostTicketInfo::getStudentName() const { return studentName_; }
const std::string& LostTicketInfo::getOwnerName() const { return ownerName_; }
const std::string& LostTicketInfo::getOwnerId() const { return ownerId_; }
const std::string& LostTicketInfo::getClassName() const { return className_; }
const std::string& LostTicketInfo::getPhone() const { return phone_; }
const std::string& LostTicketInfo::getNote() const { return note_; }
