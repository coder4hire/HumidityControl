#pragma once

#include <NimBLEDevice.h>
#include <memory>
#include <chrono>

#define SCAN_TIME 10

struct SensorData
{
    float temp = 0;
    float humi = 0;
    float voltage = 0;
    std::chrono::system_clock::time_point timestamp;
};

class BLEClientExt : public NimBLEClientCallbacks
{
public:
    static BLEClientExt &getInstance()
    {
        if (!pInst)
        {
            pInst.reset(new BLEClientExt);
        }
        return *pInst;
    }

    bool isConnected() { return pClient->isConnected(); }

    bool connectAndRegisterNotifications(NimBLEAddress htSensorAddress)
    {
        Serial.println("callback...");
        if (pClient->connect(htSensorAddress))
        {
            Serial.println("+++ Connected...");

            NimBLERemoteService *pRemoteService = pClient->getService(serviceUUID);
            if (pRemoteService == nullptr)
            {
                Serial.print(" - Failed to find our service UUID: ");
                Serial.println(serviceUUID.toString().c_str());
                disconnect();
                return false;
            }
            Serial.println("got serivce...");
            Serial.println(pRemoteService->toString().c_str());

            pRemoteService->getCharacteristics(true);
            // Obtain a reference to the characteristic in the service of the remote BLE server.
            NimBLERemoteCharacteristic *pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
            if (pRemoteCharacteristic == nullptr)
            {
                Serial.print(" - Failed to find our characteristic UUID: ");
                Serial.println(charUUID.toString().c_str());
                disconnect();
                return false;
            }
            Serial.println("registering...");
            pRemoteCharacteristic->registerForNotify(notifyCallback);
            Serial.println("connect done...");
            //disconnect();
            return true;
        }
        return false;
    }

    void disconnect()
    {
        pClient->disconnect();
    }

    SensorData getData()
    {
        return data;
    }

protected:
    static std::unique_ptr<BLEClientExt> pInst;
    NimBLEClient *pClient;

    void onConnect(NimBLEClient *pclient)
    {
        Serial.printf(" * Connected %s\n", pclient->getPeerAddress().toString().c_str());
    }

    void onDisconnect(NimBLEClient *pclient)
    {
        Serial.printf(" * Disconnected %s\n", pclient->getPeerAddress().toString().c_str());
    }

    static void notifyCallback(
        NimBLERemoteCharacteristic *pBLERemoteCharacteristic,
        uint8_t *pData,
        size_t length,
        bool isNotify)
    {
        SensorData& dt = getInstance().data;
        Serial.printf("*** Callback is called: %d\n",length);
        dt.temp = (pData[0] | (pData[1] << 8)) * 0.01; //little endian
        dt.humi = pData[2];
        dt.voltage = (pData[3] | (pData[4] << 8)) * 0.001; //little endian
        getInstance().disconnect();
    }

    SensorData data;

private:
    BLEUUID serviceUUID;
    BLEUUID charUUID;

    BLEClientExt() :
        serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6"),
        charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6")
        //charUUID("ebe0ccb7-7a0a-4b0c-8a1a-6ff2997da3a6")
    {
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(this);
    }
};

class HumSensors
{
public:
    static void init();
    static void refreshData();

protected:
    static NimBLEScan *pBLEScan;
    static std::vector<std::string> addresses;
    static std::map<std::string, SensorData> sensorsData;
};

void startBLETask( void );