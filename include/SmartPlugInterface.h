#pragma once
#include "arduino.h"

struct SmartPlugReadings
{
    int power = -1;
    int voltage = 0;

    bool isEmpty() { return power == -1; }
    bool isOn() { return voltage > 10; }
};

class SmartPlugInterface
{
public:
    SmartPlugInterface(const String &addr, const String &pass) : addr(addr), pass(pass) {}
    SmartPlugReadings getReadings();
    bool set(bool turnOn);

protected:
    bool sendGetRequest(String request, String &payload);
    String addr;
    String pass;
};