#pragma once
#include <ESPUI.h>
#include "DataStructures.h"
#include "HumSensors.h"

void setUpUI();
void generalCallback(Control *sender, int type);
void refreshSettings();
void updateReadingsGUI(const std::map<std::string, SensorData> &readings);
String getFoundSensorsList();
void enterWifiDetailsCallback(Control *sender, int type);
void saveCfgCallback(Control *sender, int type);
void resetCfgCallback(Control *sender, int type);
void settingsTabCallback(Control *sender, int type);
void refreshSensorsListCallback(Control *sender, int type);
void textCallback(Control *sender, int type);
void enabledCallback(Control *sender, int type);
