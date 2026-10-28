#include "pricing/FeeRule.h"

namespace { constexpr int NIGHT_START = 19 * 60; }

bool DayFeeRule::matches(const DateTime& entry, const DateTime& exit) const {
    return entry.sameDate(exit) && !(exit < entry) && exit.minutesOfDay() < NIGHT_START;
}
int DayFeeRule::calculate() const { return 3000; }

bool NightFeeRule::matches(const DateTime& entry, const DateTime& exit) const {
    return entry.sameDate(exit) && !(exit < entry) && entry.minutesOfDay() >= NIGHT_START;
}
int NightFeeRule::calculate() const { return 5000; }

bool MixedFeeRule::matches(const DateTime& entry, const DateTime& exit) const {
    return entry.sameDate(exit) && !(exit < entry) && entry.minutesOfDay() < NIGHT_START &&
           exit.minutesOfDay() >= NIGHT_START;
}
int MixedFeeRule::calculate() const { return 7000; }
