#pragma once

#include "models/ParkingLot.h"
#include <filesystem>

class ParkingRepository {
private:
    std::filesystem::path path_;

public:
    explicit ParkingRepository(std::filesystem::path path);
    bool hasData() const;
    ParkingLot load() const;
    void save(const ParkingLot& lot) const;
    const std::filesystem::path& getPath() const;
};
