/**
 * This is humidity control server built on top of the ESPUI framework
 * Sensors used: Xiaomi Mi Humidity sensor
 * Smart outlets used: Tasmota-based outlet (with power meter)
 *
 * When this program boots, it will load an SSID and password from the EEPROM.
 * The SSID is a null-terminated C string stored at EEPROM addresses 0-31
 * The password is a null-terminated C string stored at EEPROM addresses 32-95.
 * If these credentials do not work for some reason, the ESP will create an Access
 * Point wifi with the SSID HOSTNAME (defined below). You can then connect and use
 * the controls on the "Wifi Credentials" tab to store credentials into the EEPROM.
 *
 */

#include <Arduino.h>
#include <EEPROM.h>

#include <WiFi.h>
#include <ESPmDNS.h>

#include "HumSensors.h"
#include "SmartPlugInterface.h"
#include "UI.h"
#include "Config.h"
#include "DataStructures.h"

// Settings
#define SLOW_BOOT 0
#define HOSTNAME "HUMCTL"

// Function Prototypes
void connectWifi();

bool isWifiClient = false;

std::vector<UnitData> Units(MAX_UNITS_NUM);

void setup()
{
	Serial.begin(115200);

	Serial.println("sensors init");
	// HumSensors::init();
	Serial.println("sensors init done");

	while (!Serial)
		;
	if (SLOW_BOOT)
		delay(5000); // Delay booting to give time to connect a serial monitor
	connectWifi();
	WiFi.setSleep(true); // Sleep should be enabled for Bluetooth

	//--- Loading config
	loadSystemCfg();
	HumSensors::setPollInterval(generalCfg.pollInterval);

	setUpUI();
	HumSensors::startBLETask();
}

void loop()
{
	static long unsigned lastTime = 0;

	// Send periodic updates if switcher is turned on
	auto now = millis();
	if (now > lastTime + 5000 || now < lastTime)
	{
		//		HumSensors::refreshData();
		auto readings = HumSensors::getReadings();

		for(auto& unit : Units)
		{
			if(!unit.plug.refreshReadings())
			{
				Serial.println("Error getting data from plug "+unit.plug.getAddr());
			}
		}
		updateReadingsGUI(readings);

		//		SmartPlugInterface plug(TASMOTA_PARAMS);
		//		SmartPlugReadings plugData = plug.getReadings();
		//		Serial.printf("Power=%d; Voltage=%d\n", plugData.power, plugData.voltage);

		lastTime = now;
	}

	// Simple debug UART interface
	if (Serial.available())
	{
		switch (Serial.read())
		{
		case 'w': // Print IP details
			Serial.println(WiFi.localIP());
			break;
		case 'W': // Reconnect wifi
			connectWifi();
			break;
		default:
			Serial.print('#');
			break;
		}
	}


	if(WiFi.status() != WL_CONNECTED)
	{
		connectWifi();
	}
	delay(5000);
}

// Utilities
//------------------------------------------------------------------------------------------------
void connectWifi()
{
	int connect_timeout;

	WiFi.setHostname(HOSTNAME);
	Serial.println("Begin wifi...");

	// Load credentials from EEPROM
	yield();
	String stored_ssid, stored_pass;
	loadWifiCfg(stored_ssid, stored_pass);

	// Try to connect with stored credentials, fire up an access point if they don't work.
	WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
	connect_timeout = 28; // 7 seconds
	while (WiFi.status() != WL_CONNECTED && connect_timeout > 0)
	{
		delay(250);
		Serial.print(".");
		connect_timeout--;
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		Serial.println(WiFi.localIP());
		Serial.println("Wifi started");

		if (!MDNS.begin(HOSTNAME))
		{
			Serial.println("Error setting up MDNS responder!");
		}
		isWifiClient=true;
	}
	else if(!isWifiClient)
	{
		Serial.println("\nCreating access point...");
		WiFi.mode(WIFI_AP);
		WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
		WiFi.softAP(HOSTNAME);

		connect_timeout = 20;
		do
		{
			delay(250);
			Serial.print(",");
			connect_timeout--;
		} while (connect_timeout);
	}
}

