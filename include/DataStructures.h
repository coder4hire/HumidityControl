#pragma once
#include "SmartPlugInterface.h"

#define LOG_INFO

#ifdef LOG_INFO
#define LOG(x, ...) Serial.printf(x "\n", ##__VA_ARGS__)
#else
#define LOG(x, ...)
#endif

#define LOGERR(x, ...) Serial.printf("ERR: " x "\n", ##__VA_ARGS__)

#define MAX_UNITS_NUM 3

struct UnitConfig
{
	bool isEnabled = false;
	char name[32] = "";
	char addr[32] = "";
	int minThr = 0;
	int maxThr = 0;
	char plugAddr[128] = "";
	char plugPwd[32] = "";
	unsigned int enStartTime = 0;
	unsigned int enEndTime = 0;
	
	String getEnStartTime() 
	{
		char buf[20];
		sprintf(buf,"%02d:%02d:%02d",enStartTime/3600,(enStartTime%3600)/60,enStartTime%60);
		return String(buf);
	}

	String getEnEndTime() 
	{
		char buf[20];
		sprintf(buf,"%02d:%02d:%02d",enEndTime/3600,(enEndTime%3600)/60,enEndTime%60);
		return String(buf);
	}

	void setEnStartTime(String strTime) 
	{
		int h=0,m=0,s=0;
		sscanf(strTime.c_str(),"%d:%d:%d",&h,&m,&s);
		enStartTime=h*3600+m*60+s;		
	}

	void setEnEndTime(String strTime) 
	{
		int h=0,m=0,s=0;
		sscanf(strTime.c_str(),"%d:%d:%d",&h,&m,&s);
		enEndTime=h*3600+m*60+s;		
	}

};

struct GeneralConfig
{
	int pollInterval = 10;
	int UTCOffset=0;
	char NTPServer[128]="pool.ntp.org";
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
	int16_t idPlugPwd = 0;
	int16_t idEnStartTime;
	int16_t idEnEndTime;
};

struct UnitData
{
	UnitIDs IDs;
	UnitConfig cfg;
	SmartPlugInterface plug;
	char label[32];
};

struct GeneralCfgIDs
{
	int16_t idPollInterval = 0;
	int16_t idFoundSensors = 0;
	int16_t idSave = 0;
	int16_t idReset = 0;
	int16_t idSaveInfo = 0;
	int16_t idRefresh = 0;
	int16_t idSSID=0;
	int16_t idPass=0;
	int16_t idUTCOffset=0;
	int16_t idNTPServer=0;

	int16_t idTime=0;
};

extern std::vector<UnitData> Units;