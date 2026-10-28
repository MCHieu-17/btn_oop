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
