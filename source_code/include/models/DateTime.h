#pragma once

#include <string>

// Thời gian địa phương, độ chính xác đến phút; không phụ thuộc múi giờ khi lưu tệp.
class DateTime {
private:
    int year_, month_, day_, hour_, minute_;

public:
    DateTime(int year, int month, int day, int hour, int minute);
    static DateTime parse(const std::string& text);
    static DateTime now();
    std::string toString() const;
    std::string dateString() const;
    int minutesOfDay() const;
    bool sameDate(const DateTime& other) const;
    bool operator<(const DateTime& other) const;
    bool operator==(const DateTime& other) const;
    bool operator!=(const DateTime& other) const;
};
