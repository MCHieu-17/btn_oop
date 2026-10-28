#include "pricing/FeeCalculator.h"
#include "exceptions/ParkingException.h"

FeeCalculator::FeeCalculator() {
    rules_.push_back(std::make_unique<DayFeeRule>());
    rules_.push_back(std::make_unique<NightFeeRule>());
    rules_.push_back(std::make_unique<MixedFeeRule>());
}

int FeeCalculator::calculate(const DateTime& entry, const DateTime& exit) const {
    if (exit < entry) throw ValidationException("Giờ ra không được trước giờ vào.");
    if (!entry.sameDate(exit)) throw ValidationException("Lượt qua ngày cần nhập phí và lý do.");
    for (const auto& rule : rules_) {
        // Lời gọi qua lớp cha được phân giải tới đối tượng lớp con tại thời điểm chạy.
        if (rule->matches(entry, exit)) return rule->calculate();
    }
    throw ValidationException("Không tìm thấy quy tắc tính phí phù hợp.");
}
