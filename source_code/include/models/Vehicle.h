#pragma once

#include <string>

enum class VehicleType { Motorbike = 1, ElectricMotorbike = 2 };

class Vehicle {
private:
    std::string plate_;
    VehicleType type_;

public:
    Vehicle(const std::string& plate, VehicleType type);
    static std::string normalizePlate(const std::string& plate);
    const std::string& getPlate() const;
    VehicleType getType() const;
    std::string typeName() const;
};

// Mã thẻ được chuẩn hóa để tránh coi c001 và C001 là hai thẻ khác nhau.
std::string normalizeCardId(const std::string& text);
std::string trim(const std::string& text);
