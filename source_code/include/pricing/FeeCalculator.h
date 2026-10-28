#pragma once

#include "pricing/FeeRule.h"
#include <memory>
#include <vector>

class FeeCalculator {
private:
    std::vector<std::unique_ptr<FeeRule>> rules_;

public:
    FeeCalculator();
    int calculate(const DateTime& entry, const DateTime& exit) const;
};
