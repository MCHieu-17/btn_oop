class ParkingException : public std::runtime_error {
public:
    explicit ParkingException(const std::string& message) : std::runtime_error(message) {}
};

class ValidationException : public ParkingException { using ParkingException::ParkingException; };
class NotFoundException : public ParkingException { using ParkingException::ParkingException; };
class ConflictException : public ParkingException { using ParkingException::ParkingException; };
class CapacityException : public ParkingException { using ParkingException::ParkingException; };
class StorageException : public ParkingException { using ParkingException::ParkingException; };
