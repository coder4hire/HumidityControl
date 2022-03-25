#pragma once
#include "arduino.h"
#include <mutex>

struct SmartPlugReadings
{
    int power = -1;
    int voltage = 0;

    bool isEmpty() const { return power == -1; }
    bool isOn() const { return voltage > 10; }
    bool isLoaded() const { return power > 3; }
};

class SmartPlugInterface
{
public:
    SmartPlugInterface() {}
    void setCredentials(const String &addr, const String &pass)
    {
        std::lock_guard<std::mutex> lock(mtx);
        this->addr = addr;
        this->pass = pass;
    }
    SmartPlugReadings getReadings()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return lastReadings;
    }
    bool refreshReadings();
    bool set(bool turnOn, bool force = false);
    bool isTurnedOn()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return isTurnedOnVal;
    }
    String getAddr()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return addr;
    }

protected:
    std::mutex mtx;
    bool sendGetRequest(String request, String &payload);
    String addr;
    String pass;
    SmartPlugReadings lastReadings;
    bool isTurnedOnVal = false;
};