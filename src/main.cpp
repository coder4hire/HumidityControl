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
#include <esp_task_wdt.h>

#include "HumSensors.h"
#include "SmartPlugInterface.h"
#include "UI.h"
#include "Config.h"
#include "DataStructures.h"
//#include <chrono>
#include <time.h>

// Settings
#define HOSTNAME "HUMCTL"
#define WDT_TIMEOUT 30

// Function Prototypes
void connectWifi();

bool isWifiClient = false;

std::vector<UnitData> Units(MAX_UNITS_NUM);

bool checkRules()
{
	const auto &readings = HumSensors::getReadings();
	int retVal = 0;
	for (auto &unit : Units)
	{
		try
		{
			float hum = readings.at(unit.cfg.addr).humidity;
			struct tm tmInfo;
			int nowTime = getLocalTime(&tmInfo) ? tmInfo.tm_hour * 3600 + tmInfo.tm_min * 60 + tmInfo.tm_sec : -1;

			bool doesFitIntoEnabledTime = unit.cfg.enStartTime == unit.cfg.enEndTime || (nowTime >= 0 &&
				  ((unit.cfg.enStartTime < unit.cfg.enEndTime && nowTime >= unit.cfg.enStartTime && nowTime < unit.cfg.enEndTime) ||
				   (unit.cfg.enStartTime > unit.cfg.enEndTime && (nowTime >= unit.cfg.enStartTime || nowTime < unit.cfg.enEndTime))));
			
			if (unit.cfg.isEnabled && hum > 0 && doesFitIntoEnabledTime)
			{
				if (unit.cfg.minThr < unit.cfg.maxThr)
				{
					if (hum >= unit.cfg.maxThr)
					{
						retVal += unit.plug.set(false);
					}
					else if (hum <= unit.cfg.minThr)
					{
						retVal += unit.plug.set(true);
					}
				}
				else
				{
					unit.plug.set(false);
				}
			}
			else
			{
				unit.plug.set(false);
			}
		}
		catch (...)
		{
		}
	}
	return retVal != 0;
}

void refreshPlugsData()
{
	for (auto &unit : Units)
	{
		if (*unit.cfg.plugAddr && !unit.plug.refreshReadings())
		{
			LOGERR("Can't get data from plug %s", unit.plug.getAddr().c_str());
		}
	}
}

void setup()
{
	//ESPUI.prepareFileSystem(); // Run only once
	Serial.begin(115200);

	Serial.println("sensors init");
	// HumSensors::init();
	Serial.println("sensors init done");

	for (int i = 0; !Serial && i < 10; i++)
	{
		delay(500);
	}

	connectWifi();
	WiFi.setSleep(true); // Sleep should be enabled for Bluetooth

	//--- Loading config
	loadSystemCfg();
	HumSensors::setPollInterval(generalCfg.pollInterval);

	setUpUI();
	HumSensors::startBLETask();

	esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
	esp_task_wdt_add(NULL);				  // add current thread to WDT watch

	// init and get the time
	configTime(generalCfg.UTCOffset * 3600, 0, generalCfg.NTPServer);
}

void loop()
{
	static long unsigned lastTime = 0;

	// Send periodic updates if switcher is turned on
	auto now = millis();
	if (now > lastTime + 5000 || now < lastTime)
	{
		const auto &readings = HumSensors::getReadings();
		refreshPlugsData();
		if (checkRules())
		{
			delay(1000);
			refreshPlugsData();
		}
		updateReadingsGUI(readings);

		if (WiFi.status() != WL_CONNECTED)
		{
			connectWifi();
		}
		lastTime = now;
	}

	// Simple debug UART interface
	if (Serial.available())
	{
		switch (Serial.read())
		{
		case 'i': // Print IP details
			Serial.println(WiFi.localIP());
			break;
		case 'w': // Reconnect wifi
			connectWifi();
			break;
		default:
			Serial.print('#');
			break;
		}
	}

	esp_task_wdt_reset();
	delay(2000);
}

// Utilities
//------------------------------------------------------------------------------------------------
void connectWifi()
{
	int connect_timeout;

	WiFi.setHostname(HOSTNAME);
	LOG("Begin wifi...");

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
	Serial.print("\n");

	if (WiFi.status() == WL_CONNECTED)
	{
		LOG("Wifi (client) started, IP: %s", WiFi.localIP().toString().c_str());

		if (!MDNS.begin(HOSTNAME))
		{
			LOGERR("Error setting up MDNS responder!");
		}
		isWifiClient = true;
	}
	else if (!isWifiClient)
	{
		LOGERR("Cannot create client connection, creating access point...");
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
		Serial.print("\n");
	}
}
