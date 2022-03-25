#include "SmartPlugInterface.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DataStructures.h>

bool SmartPlugInterface::refreshReadings()
{
    std::lock_guard<std::mutex> lock(mtx);
    String payload;
    StaticJsonDocument<512> doc;
    if (sendGetRequest("cmnd=Status%208", payload))
    {
        if (deserializeJson(doc, payload) == DeserializationError::Ok)
        {
            JsonObject energyObj = doc["StatusSNS"]["ENERGY"];
            lastReadings.power = energyObj["Power"];
            lastReadings.voltage = energyObj["Voltage"];
            if (lastReadings.isOn() != isTurnedOnVal)
            {
                isTurnedOnVal = lastReadings.isOn();
            }
            return true;
        }
        else
        {
            LOGERR("Plug JSON parsing error");
        }
    }
    return false;
}

bool SmartPlugInterface::set(bool turnOn, bool force)
{
    std::lock_guard<std::mutex> lock(mtx);
    String payload;
    if (force || turnOn != isTurnedOnVal)
    {
        if (sendGetRequest(String("cmnd=Power%20") + (turnOn ? "1" : "0"), payload))
        {
            if ((turnOn && payload == "{\"POWER\":\"ON\"}") ||
                (!turnOn && payload == "{\"POWER\":\"OFF\"}"))
            {
                isTurnedOnVal = turnOn;
                return true;
            }
            else
            {
                LOGERR("Invalid payload: %s", payload.c_str());
            }
        }
    }
    return false;
}

bool SmartPlugInterface::sendGetRequest(String request, String &payload)
{
    bool retVal = false;
    if (addr == "")
    {
        return false;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.setTimeout(5000);

        String serverPath = String("http://") + addr + "/cm?" +
                            (pass != "" ? "user=admin&password=" + pass + "&" : "") + request;

        // Your Domain name with URL path or IP address with path
        http.begin(serverPath.c_str());

        // Send HTTP GET request
        int httpResponseCode = http.GET();

        if (httpResponseCode != 200)
        {
            LOGERR("Plug HTTP Error code: %d", httpResponseCode);
        }
        else
        {
            LOG("Got response from plug: %s", addr.c_str());
            payload = http.getString();
            retVal = true;
        }
        // Free resources
        http.end();
    }
    else
    {
        LOGERR("sendGetReuest: WiFi disconnected");
    }
    return retVal;
}
