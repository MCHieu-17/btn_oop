#pragma once

#include "models/ParkingSession.h"
#include <vector>

class ParkingLot {
private:
    int capacity_;
    long long nextId_;
    std::vector<ParkingSession> sessions_;
    void validate() const;

public:
    explicit ParkingLot(int capacity);
    ParkingLot(int capacity, long long nextId, std::vector<ParkingSession> sessions);
    int getCapacity() const;
    long long getNextId() const;
    std::size_t activeCount() const;
    const std::vector<ParkingSession>& getSessions() const;
    const ParkingSession& findActiveByCard(const std::string& cardId) const;
    long long recordEntry(const std::string& cardId, const Vehicle& vehicle, const DateTime& entry);
    void recordExit(long long id, const DateTime& exit, long long fee,
                    bool manual, const std::string& reason);
    void changeCapacity(int capacity);
};
