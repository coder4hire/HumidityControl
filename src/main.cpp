
/**
 * @file completeExample.cpp
 * @author Ian Gray @iangray1000
 * 
 * This is an example GUI to show off all of the features of ESPUI. 
 * This can be built using the Arduino IDE, or PlatformIO.
 * 
 * ---------------------------------------------------------------------------------------
 * If you just want to see examples of the ESPUI code, jump down to the setUpUI() function
 * ---------------------------------------------------------------------------------------
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

#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

#include "HumSensors.h"
#include "SensorsNames.h"

//Settings
#define SLOW_BOOT 0
#define HOSTNAME "HUMCTL"
#define FORCE_USE_HOTSPOT 0

//Function Prototypes
void connectWifi();
void setUpUI();
void enterWifiDetailsCallback(Control *sender, int type);
void textCallback(Control *sender, int type);
void generalCallback(Control *sender, int type);

//UI handles
uint16_t wifi_ssid_text, wifi_pass_text;
volatile bool updates = false;

struct SensorGUIData
{
	int16_t idHum = 0;
	int16_t idTemp = 0;
	int16_t idBatt = 0;
	double value = 0;
	std::string name;
};

#define MAX_SENSORS_NUM 3

std::vector<SensorGUIData> Sensors(MAX_SENSORS_NUM);

// This is the main function which builds our GUI
void setUpUI()
{
	//Turn off verbose debugging
	ESPUI.setVerbosity(Verbosity::Verbose);

	//Make sliders continually report their position as they are being dragged.
	ESPUI.sliderContinuous = true;

	//This GUI is going to be a tabbed GUI, so we are adding most controls using ESPUI.addControl
	//which allows us to set a parent control. If we didn't need tabs we could use the simpler add
	//functions like:
	//    ESPUI.button()
	//    ESPUI.label()

	/*
	 * Tab: Basic Controls
	 * This tab contains all the basic ESPUI controls, and shows how to read and update them at runtime.
	 *-----------------------------------------------------------------------------------------------------------*/
	auto statusTab = ESPUI.addControl(Tab, "", "Status");

	ESPUI.addControl(Separator, "Humidity", "", None, statusTab);
	for (int i = 0; i < Sensors.size(); i++)
	{
		Sensors[i].idHum = ESPUI.addControl(Label, Sensors[i].name.c_str(), "---", Turquoise, statusTab, generalCallback);
		Sensors[i].idTemp = ESPUI.addControl(Label, Sensors[i].name.c_str(), "---", Turquoise, Sensors[i].idHum, generalCallback);
		Sensors[i].idBatt = ESPUI.addControl(Label, Sensors[i].name.c_str(), "---", Turquoise, Sensors[i].idHum, generalCallback);
	//	ESPUI.addControl(Min, "", "10", None, Sensors[i].ctrlID);
	//	ESPUI.addControl(Max, "", "400", None, Sensors[i].ctrlID);
	}

	auto settingsTab = ESPUI.addControl(Tab, "", "Settings");

	//	mainLabel = ESPUI.addControl(Label, "Label", "Label text", Emerald, maintab, generalCallback);
	//	mainSwitcher = ESPUI.addControl(Switcher, "Switcher", "", Sunflower, maintab, generalCallback);

	//Sliders default to being 0 to 100, but if you want different limits you can add a Min and Max control
	//	mainSlider = ESPUI.addControl(Slider, "Slider", "200", Turquoise, maintab, generalCallback);
	//	ESPUI.addControl(Min, "", "10", None, mainSlider);
	//	ESPUI.addControl(Max, "", "400", None, mainSlider);

	//Number inputs also accept Min and Max components, but you should still validate the values.
	//	mainNumber = ESPUI.addControl(Number, "Number Input", "42", Emerald, maintab, generalCallback);
	//	ESPUI.addControl(Min, "", "10", None, mainNumber);
	//	ESPUI.addControl(Max, "", "50", None, mainNumber);

	//	styleSwitcher = ESPUI.addControl(Switcher, "Styled Switcher", "1", Alizarin, styletab, generalCallback);

	/*
	 * Tab: WiFi Credentials
	 * You use this tab to enter the SSID and password of a wifi network to autoconnect to.
	 *-----------------------------------------------------------------------------------------------------------*/
	auto wifitab = ESPUI.addControl(Tab, "", "WiFi Credentials");
	wifi_ssid_text = ESPUI.addControl(Text, "SSID", "", Alizarin, wifitab, textCallback);
	//Note that adding a "Max" control to a text control sets the max length
	ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
	wifi_pass_text = ESPUI.addControl(Text, "Password", "", Alizarin, wifitab, textCallback);
	ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
	ESPUI.addControl(Button, "Save", "Save", Peterriver, wifitab, enterWifiDetailsCallback);

	//Finally, start up the UI.
	//This should only be called once we are connected to WiFi.
	ESPUI.begin("  Humidity Control");
}

//Set the various controls to random value to show how controls can be updated at runtime
//		ESPUI.updateLabel(mainLabel, String(rndString1));
//		ESPUI.updateSwitcher(mainSwitcher, ESPUI.getControl(mainSwitcher)->value.toInt() ? false : true);
//		ESPUI.updateSlider(mainSlider, random(10, 400));
//		ESPUI.updateText(mainText, String(rndString2));
//		ESPUI.updateNumber(mainNumber, random(100000));

//Most elements in this test UI are assigned this generic callback which prints some
//basic information. Event types are defined in ESPUI.h
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
	//HumSensors::init();
	Serial.println("sensors init done");

	while (!Serial)
		;
	if (SLOW_BOOT)
		delay(5000); //Delay booting to give time to connect a serial monitor
	connectWifi();
	WiFi.setSleep(true); //Sleep should be enabled for Bluetooth

	//--- Set up sensors structure
	SET_SENSORS_NAMES;

	setUpUI();
	startBLETask();
}

void loop()
{
	static long unsigned lastTime = 0;

	//Send periodic updates if switcher is turned on
	auto now = millis();
	if (now > lastTime + 5000 || now < lastTime)
	{
//		HumSensors::refreshData();
		auto readings = HumSensors::getReadings();
		for(int i=0;i<Sensors.size();i++)
		{
			try
			{
				const auto& data = readings.at(Sensors[i].name);
				ESPUI.updateLabel(Sensors[i].idHum,String(data.humidity)+" %");
				ESPUI.updateLabel(Sensors[i].idTemp,String(data.temp) +" C");
				ESPUI.updateLabel(Sensors[i].idBatt,String(data.voltage)+" V");
			}
			catch(...)
			{
				ESPUI.updateLabel(Sensors[i].idHum,"- NO DATA -");
				ESPUI.updateLabel(Sensors[i].idTemp,"- NO DATA -");
				ESPUI.updateLabel(Sensors[i].idBatt,"- NO DATA -");
			}
		}
		lastTime=now;
	}

	//Simple debug UART interface
	if (Serial.available())
	{
		switch (Serial.read())
		{
		case 'w': //Print IP details
			Serial.println(WiFi.localIP());
			break;
		case 'W': //Reconnect wifi
			connectWifi();
			break;
		case 'C': //Force a crash (for testing exception decoder)
#if !defined(ESP32)
			((void (*)())0xf00fdead)();
#endif
		default:
			Serial.print('#');
			break;
		}
	}

#if !defined(ESP32)
	//We don't need to call this explicitly on ESP32 but we do on 8266
	MDNS.update();
#endif
}

//Utilities
//
//If you are here just to see examples of how to use ESPUI, you can ignore the following functions
//------------------------------------------------------------------------------------------------
void readStringFromEEPROM(String &buf, int baseaddress, int size)
{
	buf.reserve(size);
	for (int i = baseaddress; i < baseaddress + size; i++)
	{
		char c = EEPROM.read(i);
		buf += c;
		if (!c)
			break;
	}
}

void connectWifi()
{
	int connect_timeout;

#if defined(ESP32)
	WiFi.setHostname(HOSTNAME);
#else
	WiFi.hostname(HOSTNAME);
#endif
	Serial.println("Begin wifi...");

	//Load credentials from EEPROM
	if (!(FORCE_USE_HOTSPOT))
	{
		yield();
		EEPROM.begin(100);
		String stored_ssid, stored_pass;
		readStringFromEEPROM(stored_ssid, 0, 32);
		readStringFromEEPROM(stored_pass, 32, 96);
		EEPROM.end();

//Try to connect with stored credentials, fire up an access point if they don't work.
#if defined(ESP32)
		WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
#else
		WiFi.begin(stored_ssid, stored_pass);
#endif
		connect_timeout = 28; //7 seconds
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

void enterWifiDetailsCallback(Control *sender, int type)
{
	if (type == B_UP)
	{
		Serial.println("Saving credentials to EPROM...");
		Serial.println(ESPUI.getControl(wifi_ssid_text)->value);
		Serial.println(ESPUI.getControl(wifi_pass_text)->value);
		unsigned int i;
		EEPROM.begin(100);
		for (i = 0; i < ESPUI.getControl(wifi_ssid_text)->value.length(); i++)
		{
			EEPROM.write(i, ESPUI.getControl(wifi_ssid_text)->value.charAt(i));
			if (i == 30)
				break; //Even though we provided a max length, user input should never be trusted
		}
		EEPROM.write(i, '\0');

		for (i = 0; i < ESPUI.getControl(wifi_pass_text)->value.length(); i++)
		{
			EEPROM.write(i + 32, ESPUI.getControl(wifi_pass_text)->value.charAt(i));
			if (i == 94)
				break; //Even though we provided a max length, user input should never be trusted
		}
		EEPROM.write(i + 32, '\0');
		EEPROM.end();
	}
}

void textCallback(Control *sender, int type)
{
	//This callback is needed to handle the changed values, even though it doesn't do anything itself.
}