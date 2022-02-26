
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
#include <ESPUI.h>

#include <WiFi.h>
#include <ESPmDNS.h>

#include "HumSensors.h"
#include "SmartPlugInterface.h"
#include "Config.h"
#include "DataStructures.h"

// Settings
#define SLOW_BOOT 0
#define HOSTNAME "HUMCTL"
#define FORCE_USE_HOTSPOT 0

// Function Prototypes
void connectWifi();
void setUpUI();
void enterWifiDetailsCallback(Control *sender, int type);
void textCallback(Control *sender, int type);
void generalCallback(Control *sender, int type);
void enabledCallback(Control *sender, int type);
void saveCfgCallback(Control *sender, int type);
void resetCfgCallback(Control *sender, int type);
void settingsTabCallback(Control *sender, int type);
void refreshSensorsListCallback(Control *sender, int type);

void refreshSettings();
String getFoundSensorsList();

// UI handles
uint16_t wifi_ssid_text, wifi_pass_text;

Preferences prefs;

GeneralCfgIDs generalCfgIDs;
GeneralConfig generalCfg;
std::vector<UnitData> Units(MAX_UNITS_NUM);

String ledStyle = "<span style='border-radius:50%;width:15px;height:15px;display:inline-block;border:1px;border-style:solid;margin-right:5px;color:dimgray;background:";
String divStyle = "<div style='width:7em;display:inline-block'>";
#define LED_LABEL(color, text) ledStyle + #color "'/>" + divStyle + #text "</span>"

// This is the main function which builds our GUI
void setUpUI()
{
	// Turn off verbose debugging
	ESPUI.setVerbosity(Verbosity::Quiet);

	auto statusTab = ESPUI.addControl(Tab, "", "Status");

	ESPUI.addControl(Separator, "Humidity", "", None, statusTab);
	for (int i = 0; i < Units.size(); i++)
	{
		Units[i].IDs.idLabel = ESPUI.addControl(Label, !Units[i].cfg.name[0] ? Units[i].cfg.addr : Units[i].cfg.name, "---", Turquoise, statusTab, generalCallback);
		Units[i].IDs.idOn = ESPUI.addControl(Label, "", LED_LABEL(gray, Unknown), Turquoise, Units[i].IDs.idLabel, generalCallback);
		Units[i].IDs.idWater = ESPUI.addControl(Label, "", LED_LABEL(gray, Unknown), Turquoise, Units[i].IDs.idLabel, generalCallback);
	}

	auto settingsTab = ESPUI.addControl(Tab, "", "Settings", None, Control::noParent, settingsTabCallback);
	ESPUI.addControl(Separator, "Units Settings", "", None, settingsTab);

	String clearLabelStyle = "background-color: unset; width: 100%;";
	String newLineStyle = "background-color: unset; width: 100%;";
	// String ctrlStyle = "width: 80% !important;";
	for (int i = 0; i < Units.size(); i++)
	{
		sprintf(Units[i].label, "Unit %d", i);
		Units[i].IDs.idEnabled = ESPUI.addControl(Switcher, Units[i].label, Units[i].cfg.isEnabled ? "1" : "0", Wetasphalt, settingsTab, enabledCallback);
		const auto &root = Units[i].IDs.idEnabled;
		ESPUI.setElementStyle(ESPUI.addControl(Label, "", "", None, root), newLineStyle);

		ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Name : ", None, root), clearLabelStyle);
		Units[i].IDs.idName = ESPUI.addControl(Text, "", Units[i].cfg.name, None, root, textCallback);
		ESPUI.addControl(Max, "", "31", None, Units[i].IDs.idName);
		//	ESPUI.setElementStyle(Units[i].IDs.idName, ctrlStyle);

		ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Addr : ", None, root), clearLabelStyle);
		Units[i].IDs.idAddr = ESPUI.addControl(Text, "", Units[i].cfg.addr, None, root, textCallback);
		ESPUI.addControl(Max, "", "31", None, Units[i].IDs.idAddr);
		//	ESPUI.setElementStyle(Units[i].IDs.idAddr, ctrlStyle);

		ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Min : ", None, root), clearLabelStyle);
		Units[i].IDs.idMin = ESPUI.addControl(Number, "", String(Units[i].cfg.minThr), None, root, generalCallback);
		ESPUI.addControl(Min, "", "0", None, Units[i].IDs.idMin);
		ESPUI.addControl(Max, "", "100", None, Units[i].IDs.idMin);

		ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Max : ", None, root), clearLabelStyle);
		Units[i].IDs.idMax = ESPUI.addControl(Number, "", String(Units[i].cfg.maxThr), None, root, generalCallback);
		ESPUI.addControl(Min, "", "0", None, Units[i].IDs.idMax);
		ESPUI.addControl(Max, "", "100", None, Units[i].IDs.idMax);

		ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Smart Plug : ", None, root), clearLabelStyle);
		Units[i].IDs.idPlugAddr = ESPUI.addControl(Text, "", Units[i].cfg.plugAddr, None, root, textCallback);
		ESPUI.addControl(Max, "", "127", None, Units[i].IDs.idAddr);
	}

	generalCfgIDs.idPollInterval = ESPUI.addControl(Number, "Poll Interval", String(generalCfg.pollInterval), None, settingsTab, textCallback);
	ESPUI.addControl(Min, "", "10", None, generalCfgIDs.idPollInterval);
	ESPUI.addControl(Max, "", "32000", None, generalCfgIDs.idPollInterval);

	generalCfgIDs.idSave = ESPUI.addControl(Button, "Save /Reset", "Save", Peterriver, settingsTab, saveCfgCallback);
	generalCfgIDs.idReset = ESPUI.addControl(Button, "", "Reset", Peterriver, generalCfgIDs.idSave, resetCfgCallback);

	ESPUI.addControl(Separator, "Found Sensors", "", None, settingsTab);
	generalCfgIDs.idFoundSensors = ESPUI.addControl(Label, "Found Sensors", getFoundSensorsList(), Turquoise, settingsTab, generalCallback);
	generalCfgIDs.idRefresh = ESPUI.addControl(Button, "", "Refresh", Turquoise, generalCfgIDs.idFoundSensors, refreshSensorsListCallback);

	/*
	 * Tab: WiFi Credentials
	 * You use this tab to enter the SSID and password of a wifi network to autoconnect to.
	 *-----------------------------------------------------------------------------------------------------------*/
	auto wifitab = ESPUI.addControl(Tab, "", "WiFi Credentials");
	wifi_ssid_text = ESPUI.addControl(Text, "SSID", "", Alizarin, wifitab, textCallback);
	// Note that adding a "Max" control to a text control sets the max length
	ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
	wifi_pass_text = ESPUI.addControl(Text, "Password", "", Alizarin, wifitab, textCallback);
	ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
	ESPUI.addControl(Button, "Save", "Save", Peterriver, wifitab, enterWifiDetailsCallback);

	// Finally, start up the UI.
	// This should only be called once we are connected to WiFi.
	ESPUI.begin("Humidity Control");
}

// Set the various controls to random value to show how controls can be updated at runtime
//		ESPUI.updateLabel(mainLabel, String(rndString1));
//		ESPUI.updateSwitcher(mainSwitcher, ESPUI.getControl(mainSwitcher)->value.toInt() ? false : true);
//		ESPUI.updateSlider(mainSlider, random(10, 400));
//		ESPUI.updateText(mainText, String(rndString2));
//		ESPUI.updateNumber(mainNumber, random(100000));

// Most elements in this test UI are assigned this generic callback which prints some
// basic information. Event types are defined in ESPUI.h
void generalCallback(Control *sender, int type)
{
	Serial.print("CB: id(");
	Serial.print(sender->id);
	Serial.print(") Type(");
	Serial.print(type);
	Serial.print(") '");
	Serial.print(sender->label);
	Serial.print("' = ");
	Serial.println(sender->value);
}

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
	if (now > lastTime + 50000 || now < lastTime)
	{
		//		HumSensors::refreshData();
		auto readings = HumSensors::getReadings();
		for (int i = 0; i < Units.size(); i++)
		{
			try
			{
				const auto &data = readings.at(Units[i].cfg.addr);
				ESPUI.updateLabel(Units[i].IDs.idLabel, String((int)data.humidity) + "%    " + String(data.temp, 1) + "&#176C    (" +
															String(data.voltage) + " V)");
				ESPUI.updateLabel(Units[i].IDs.idOn, LED_LABEL(green, Spraying));
				ESPUI.updateLabel(Units[i].IDs.idWater, LED_LABEL(blue, Water));
			}
			catch (...)
			{
				ESPUI.updateLabel(Units[i].IDs.idLabel, "- NO DATA -");
				ESPUI.updateLabel(Units[i].IDs.idOn, LED_LABEL(gray, Unknown));
				ESPUI.updateLabel(Units[i].IDs.idWater, LED_LABEL(gray, Unknown));
			}
		}

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
		case 'C': // Force a crash (for testing exception decoder)
#if !defined(ESP32)
			((void (*)())0xf00fdead)();
#endif
		default:
			Serial.print('#');
			break;
		}
	}

#if !defined(ESP32)
	// We don't need to call this explicitly on ESP32 but we do on 8266
	MDNS.update();
#endif
}

// Utilities
//
// If you are here just to see examples of how to use ESPUI, you can ignore the following functions
//------------------------------------------------------------------------------------------------
void connectWifi()
{
	int connect_timeout;

	WiFi.setHostname(HOSTNAME);
	Serial.println("Begin wifi...");

	// Load credentials from EEPROM
	if (!(FORCE_USE_HOTSPOT))
	{
		yield();
		String stored_ssid, stored_pass;
		loadWifiCfg(stored_ssid, stored_pass);

// Try to connect with stored credentials, fire up an access point if they don't work.
#if defined(ESP32)
		WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
#else
		WiFi.begin(stored_ssid, stored_pass);
#endif
		connect_timeout = 28; // 7 seconds
		while (WiFi.status() != WL_CONNECTED && connect_timeout > 0)
		{
			delay(250);
			Serial.print(".");
			connect_timeout--;
		}
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		Serial.println(WiFi.localIP());
		Serial.println("Wifi started");

		if (!MDNS.begin(HOSTNAME))
		{
			Serial.println("Error setting up MDNS responder!");
		}
	}
	else
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

void refreshSettings()
{
	for (int i = 0; i < Units.size(); i++)
	{
		ESPUI.updateControlValue(Units[i].IDs.idEnabled, Units[i].cfg.isEnabled ? "1" : "0");
		ESPUI.updateControlValue(Units[i].IDs.idName, Units[i].cfg.name);
		ESPUI.updateControlValue(Units[i].IDs.idAddr, Units[i].cfg.addr);
		ESPUI.updateControlValue(Units[i].IDs.idMin, String(Units[i].cfg.minThr));
		ESPUI.updateControlValue(Units[i].IDs.idMax, String(Units[i].cfg.maxThr));
	}
	ESPUI.updateControlValue(generalCfgIDs.idPollInterval, String(generalCfg.pollInterval));
}

String getFoundSensorsList()
{
	auto readings = HumSensors::getReadings();
	String retVal;
	for (const auto &data : readings)
	{
		retVal += String(data.first.c_str()) + " (" + String(data.second.temp, 1) + " / " + String((int)data.second.humidity) + "% )\n";
	}
	return retVal != "" ? retVal : "--- No Sensors Found ---";
}

void enterWifiDetailsCallback(Control *sender, int type)
{
	if (type == B_UP)
	{
		Serial.println("Saving credentials to EPROM...");
		saveWifiCfg(ESPUI.getControl(wifi_ssid_text)->value, ESPUI.getControl(wifi_pass_text)->value);
	}
}

void saveCfgCallback(Control *sender, int type)
{
	if (type == B_UP)
	{
		for (int i = 0; i < Units.size(); i++)
		{
			Units[i].cfg.isEnabled = ESPUI.getControl(Units[i].IDs.idEnabled)->value == "1";
			strncpy(Units[i].cfg.name, ESPUI.getControl(Units[i].IDs.idName)->value.c_str(), sizeof(Units[i].cfg.name));
			Units[i].cfg.name[sizeof(Units[i].cfg.name) - 1] = 0;
			strncpy(Units[i].cfg.addr, ESPUI.getControl(Units[i].IDs.idAddr)->value.c_str(), sizeof(Units[i].cfg.addr));
			Units[i].cfg.addr[sizeof(Units[i].cfg.addr) - 1] = 0;
			Units[i].cfg.minThr = atoi(ESPUI.getControl(Units[i].IDs.idMin)->value.c_str());
			Units[i].cfg.maxThr = atoi(ESPUI.getControl(Units[i].IDs.idMax)->value.c_str());
		}

		generalCfg.pollInterval = atoi(ESPUI.getControl(generalCfgIDs.idPollInterval)->value.c_str());
		if (generalCfg.pollInterval > 0 && generalCfg.pollInterval <= 32000)
		{
			HumSensors::setPollInterval(generalCfg.pollInterval);
		}
		saveSystemCfg();
	}
}

void resetCfgCallback(Control *sender, int type)
{
	if (type == B_UP)
	{
		refreshSettings();
	}
}

void settingsTabCallback(Control *sender, int type)
{
	if (type == B_UP)
	{
		refreshSettings();
	}
}

void refreshSensorsListCallback(Control *sender, int type)
{
	if (type == B_UP)
	{
		ESPUI.updateControlValue(generalCfgIDs.idFoundSensors, getFoundSensorsList());
	}
}

void textCallback(Control *sender, int type)
{
	// This callback is needed to handle the changed values, even though it doesn't do anything itself.
}

void enabledCallback(Control *sender, int type)
{
}
