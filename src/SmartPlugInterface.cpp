#include "SmartPlugInterface.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

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
            return true;
        }
        else
        {
            // Serial.printf("Plug JSON parsing error, payload:\n%s\n",payload.c_str());
            Serial.printf("Plug JSON parsing error\n");
        }
    }
    return false;
}

bool SmartPlugInterface::set(bool turnOn)
{
    std::lock_guard<std::mutex> lock(mtx);    
    String payload;
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
            Serial.printf("Invalid payload: %s\n", payload.c_str());
        }
    }
    return false;
}

bool SmartPlugInterface::sendGetRequest(String request, String &payload)
{
    bool retVal = false;
    if(addr=="")
    {
        return false;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        String serverPath = String("http://") + addr + "/cm?" +
                            (pass != "" ? "user=admin&password=" + pass + "&" : "") + request;

        // Your Domain name with URL path or IP address with path
        http.begin(serverPath.c_str());

        // Send HTTP GET request
        int httpResponseCode = http.GET();

        if (httpResponseCode != 200)
        {
            Serial.printf("Plug HTTP Error code: %d\n", httpResponseCode);
        }
        else
        {
            payload = http.getString();
            retVal = true;
        }
        // Free resources
        http.end();
    }
    else
    {
        Serial.println("WiFi disconnected");
    }
    return retVal;
}
