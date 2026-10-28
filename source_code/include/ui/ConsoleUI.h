#pragma once

#include "services/ParkingService.h"

class ConsoleUI {
private:
    ParkingService& service_;
    bool demo_;
    DateTime readTime(const std::string& label) const;
    void receiveVehicle();
    void releaseVehicle();
    void showActive() const;
    void search() const;
    void showHistory() const;
    void showStatistics() const;
    void changeCapacity();
    static void printSession(const ParkingSession& session);

public:
    ConsoleUI(ParkingService& service, bool demo);
    void run();
    static std::string readLine(const std::string& prompt);
    static long long readNumber(const std::string& prompt, long long min, long long max);
    static bool confirm(const std::string& prompt);
    static std::string money(long long amount);
};
