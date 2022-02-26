#pragma once

#include <NimBLEDevice.h>
#include <memory>
#include <chrono>
#include <mutex>
#include <atomic>

#define SCAN_TIME 10

struct SensorData
{
    float temp = 0;
    float humidity = 0;
    float voltage = 0;
    std::chrono::system_clock::time_point timestamp;
};

class BLEClientExt;

class HumSensors
{
public:
    static void init();
    static void refreshClientsList();
    static void refreshData();
    static std::map<std::string, SensorData> getReadings();
    static void setReadings(NimBLEAddress addr, SensorData data);
    static void startBLETask(void);
    static void setPollInterval(int value){pollInterval=value;}

protected:
    static NimBLEScan *pBLEScan;
    static std::map<NimBLEAddress, std::unique_ptr<BLEClientExt>> clients;
    static std::map<std::string, SensorData> readings;
    static std::mutex mtx;
    static std::atomic_int pollInterval;

    static void vTaskCode(void *pvParameters);
};


class BLEClientExt
{
public:
    BLEClientExt(NimBLEAddress htSensorAddress) : peerAddress(htSensorAddress),
        serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6"),
        charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6")
    {
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCBs());
    }

    BLEClientExt(const BLEClientExt &) = delete;
    BLEClientExt &operator=(const BLEClientExt &) = delete;

    virtual ~BLEClientExt()
    {
        NimBLEDevice::deleteClient(pClient);
    }

    bool isConnected() { return pClient->isConnected(); }

    NimBLEAddress getPeerAddress() { return pClient->getPeerAddress(); }

    bool connectAndRegisterNotifications()
    {
        if (pClient->connect(peerAddress,false))
        {
            NimBLERemoteService *pRemoteService = pClient->getService(serviceUUID);
            if (pRemoteService == nullptr) {
                // Perform full service refresh
                pClient->getServices(true);
                pRemoteService = pClient->getService(serviceUUID);
            }

            if (pRemoteService == nullptr)
            {
                Serial.print(" - Failed to find our service UUID: ");
                Serial.println(serviceUUID.toString().c_str());
                disconnect();
                return false;
            }

            //  Obtain a reference to the characteristic in the service of the remote BLE server.
            NimBLERemoteCharacteristic *pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
            if (pRemoteCharacteristic == nullptr)
            {
                Serial.print(" - Failed to find our characteristic UUID: ");
                Serial.println(charUUID.toString().c_str());
                disconnect();
                return false;
            }
            pRemoteCharacteristic->subscribe(true,notifyCallback);
            return true;
        }
        return false;
    }

    void disconnect()
    {
        if(isConnected())
        {
            pClient->disconnect();
        }
    }

    SensorData getData()
    {
        return data;
    }

protected:
    NimBLEAddress peerAddress;
    static std::unique_ptr<BLEClientExt> pInst;
    NimBLEClient *pClient;

    class ClientCBs : public NimBLEClientCallbacks
    {
    public:
        ~ClientCBs()
        {
            Serial.println("Callbacks destroyed");
        }
        void onConnect(NimBLEClient *pclient)
        {
            Serial.printf(" * Connected %s\n", pclient->getPeerAddress().toString().c_str());
        }

        void onDisconnect(NimBLEClient *pclient)
        {
            Serial.printf(" * Disconnected %s\n", pclient->getPeerAddress().toString().c_str());
        }
    };

    static void notifyCallback(
        NimBLERemoteCharacteristic *pBLERemoteCharacteristic,
        uint8_t *pData,
        size_t length,
        bool isNotify)
    {
        NimBLEClient *pClient = pBLERemoteCharacteristic->getRemoteService()->getClient();
        Serial.printf("*** callback for client: %s\n", ((std::string)pClient->getPeerAddress()).c_str());
        SensorData dt;
        if(length>=5)
        {
            dt.temp = (pData[0] | (pData[1] << 8)) * 0.01; // little endian
            dt.humidity = pData[2];
            dt.voltage = (pData[3] | (pData[4] << 8)) * 0.001; // little endian
        }
        dt.timestamp = std::chrono::system_clock::now();
        HumSensors::setReadings(pClient->getPeerAddress(),dt);
        pClient->disconnect();
    }

    SensorData data;

private:
    BLEUUID serviceUUID;
    BLEUUID charUUID;
};