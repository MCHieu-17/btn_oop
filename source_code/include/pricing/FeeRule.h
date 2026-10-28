#pragma once

#include "models/DateTime.h"

// Giao diện chung cho các cách tính phí; hàm hủy ảo bảo đảm hủy đúng lớp con.
class FeeRule {
public:
    virtual bool matches(const DateTime& entry, const DateTime& exit) const = 0;
    virtual int calculate() const = 0;
    virtual ~FeeRule() = default;
};

class DayFeeRule : public FeeRule {
public:
    bool matches(const DateTime& entry, const DateTime& exit) const override;
    int calculate() const override;
};

class NightFeeRule : public FeeRule {
public:
    bool matches(const DateTime& entry, const DateTime& exit) const override;
    int calculate() const override;
};

class MixedFeeRule : public FeeRule {
public:
    bool matches(const DateTime& entry, const DateTime& exit) const override;
    int calculate() const override;
};
