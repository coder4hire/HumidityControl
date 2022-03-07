#pragma once
#define EEPROM_SYSTEM_CONFIG_OFFSET 128

#include <Preferences.h>
#include "DataStructures.h"

extern GeneralConfig generalCfg;

void saveWifiCfg(String SSID, String pwd);
void loadWifiCfg(String &SSID, String &pwd);
void saveSystemCfg();
void loadSystemCfg();
