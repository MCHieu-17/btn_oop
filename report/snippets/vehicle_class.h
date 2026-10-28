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
