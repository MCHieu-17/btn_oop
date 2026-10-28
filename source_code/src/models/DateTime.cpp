#include "models/DateTime.h"
#include "exceptions/ParkingException.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <tuple>

DateTime::DateTime(int year, int month, int day, int hour, int minute)
    : year_(year), month_(month), day_(day), hour_(hour), minute_(minute) {
    if (year < 1 || year > 9999 || month < 1 || month > 12 || hour < 0 ||
        hour > 23 || minute < 0 || minute > 59) {
        throw ValidationException("Ngày giờ không hợp lệ.");
    }
    const bool leap = (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
    const int days[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (day < 1 || day > days[month - 1]) {
        throw ValidationException("Ngày không tồn tại trong tháng.");
    }
}

DateTime DateTime::parse(const std::string& text) {
    if (text.size() != 16 || text[4] != '-' || text[7] != '-' ||
        text[10] != ' ' || text[13] != ':') {
        throw ValidationException("Nhập thời gian theo YYYY-MM-DD HH:MM.");
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13) continue;
        if (text[i] < '0' || text[i] > '9') {
            throw ValidationException("Thời gian chứa ký tự không hợp lệ.");
        }
    }
    return DateTime(std::stoi(text.substr(0, 4)), std::stoi(text.substr(5, 2)),
                    std::stoi(text.substr(8, 2)), std::stoi(text.substr(11, 2)),
                    std::stoi(text.substr(14, 2)));
}

DateTime DateTime::now() {
    const std::time_t current = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &current);
#else
    localtime_r(&current, &local);
#endif
    return DateTime(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                    local.tm_hour, local.tm_min);
}

std::string DateTime::toString() const {
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << year_ << '-' << std::setw(2) << month_
        << '-' << std::setw(2) << day_ << ' ' << std::setw(2) << hour_ << ':'
        << std::setw(2) << minute_;
    return out.str();
}

std::string DateTime::dateString() const { return toString().substr(0, 10); }
int DateTime::minutesOfDay() const { return hour_ * 60 + minute_; }
bool DateTime::sameDate(const DateTime& other) const {
    return std::tie(year_, month_, day_) == std::tie(other.year_, other.month_, other.day_);
}
bool DateTime::operator<(const DateTime& other) const {
    return std::tie(year_, month_, day_, hour_, minute_) <
           std::tie(other.year_, other.month_, other.day_, other.hour_, other.minute_);
}
bool DateTime::operator==(const DateTime& other) const {
    return std::tie(year_, month_, day_, hour_, minute_) ==
           std::tie(other.year_, other.month_, other.day_, other.hour_, other.minute_);
}
bool DateTime::operator!=(const DateTime& other) const { return !(*this == other); }
