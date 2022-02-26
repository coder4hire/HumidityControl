#pragma once

#define MAX_UNITS_NUM 3

struct UnitConfig
{
	bool isEnabled = false;
	char name[32] = "";
	char addr[32] = "";
	int minThr = 0;
	int maxThr = 0;
	char plugAddr[128];
};

struct GeneralConfig
{
	int pollInterval = 10;
};

struct UnitIDs
{
	// GUI
	int16_t idLabel = 0;
	int16_t idOn = 0;
	int16_t idWater = 0;

	// Settings
	int16_t idEnabled = 0;
	int16_t idName = 0;
	int16_t idAddr = 0;
	int16_t idMin = 0;
	int16_t idMax = 0;
	int16_t idPlugAddr = 0;
};

struct UnitData
{
	UnitIDs IDs;
	UnitConfig cfg;
	char label[32];
};

struct GeneralCfgIDs
{
	int16_t idPollInterval = 0;
	int16_t idFoundSensors = 0;
	int16_t idSave = 0;
	int16_t idReset = 0;
	int16_t idRefresh = 0;
};
