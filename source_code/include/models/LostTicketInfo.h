#pragma once

#include <string>

// Thông tin sinh viên và chủ xe khi sinh viên làm mất thẻ gửi xe.
// Ảnh giấy tờ xe được mô phỏng bằng thông tin nhập tay, không lưu tệp ảnh.
class LostTicketInfo {
private:
    std::string studentId_;
    std::string studentName_;
    std::string ownerName_;
    std::string ownerId_;
    std::string className_;
    std::string phone_;
    std::string note_;

public:
    LostTicketInfo(const std::string& studentId, const std::string& studentName,
                   const std::string& ownerName, const std::string& ownerId = "",
                   const std::string& className = "", const std::string& phone = "",
                   const std::string& note = "");
    const std::string& getStudentId() const;
    const std::string& getStudentName() const;
    const std::string& getOwnerName() const;
    const std::string& getOwnerId() const;
    const std::string& getClassName() const;
    const std::string& getPhone() const;
    const std::string& getNote() const;
};
