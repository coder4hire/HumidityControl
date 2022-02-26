#pragma once
#define EEPROM_SYSTEM_CONFIG_OFFSET 128

#include <Preferences.h>
#include "DataStructures.h"

extern GeneralConfig generalCfg;
extern std::vector<UnitData> Units;
extern Preferences prefs;

static void saveWifiCfg(String SSID, String pwd)
{
    prefs.begin("wifi");
    prefs.putString("SSID",SSID);
    prefs.putString("pwd",pwd);
    prefs.end();
}

static void loadWifiCfg(String &SSID, String &pwd)
{
    prefs.begin("wifi");
    SSID = prefs.getString("SSID");
    pwd = prefs.getString("pwd");
    prefs.end();
}

static void saveSystemCfg()
{
    prefs.begin("system");
    prefs.putBytes("general",&generalCfg, sizeof(generalCfg));
    for (int i=0;i<Units.size();i++)
    {
        const auto &cfg = Units[i].cfg;
        prefs.putBytes((String("unit")+String(i)).c_str(),(void*)&cfg, sizeof(UnitConfig));
    }
    prefs.end();    

    Serial.println("System config is saved");    
}

static void loadSystemCfg()
{
    prefs.begin("system");
    prefs.getBytes("general", &generalCfg, sizeof(generalCfg));

    if(generalCfg.pollInterval<0 || generalCfg.pollInterval>32000)
    {
        generalCfg.pollInterval=30;
    }

    for (int i=0;i<Units.size();i++)
    {
        auto &cfg = Units[i].cfg;
        cfg = UnitConfig();
        prefs.getBytes((String("unit")+String(i)).c_str(), &cfg, sizeof(UnitConfig));
        cfg.name[sizeof(cfg.name) - 1] = 0;
        cfg.addr[sizeof(cfg.addr) - 1] = 0;
        cfg.plugAddr[sizeof(cfg.plugAddr) - 1] = 0;

        if (cfg.minThr < 0 || cfg.minThr > 100)
        {
            cfg.minThr = 0;
        }
        if (cfg.maxThr < 0 || cfg.maxThr > 100)
        {
            cfg.maxThr = 0;
        }
    }
    prefs.end();
}