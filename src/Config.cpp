#include "Config.h"

Preferences prefs;
GeneralConfig generalCfg;

void saveWifiCfg(String SSID, String pwd)
{
    prefs.begin("wifi");
    prefs.putString("SSID",SSID);
    prefs.putString("pwd",pwd);
    prefs.end();
}

void loadWifiCfg(String &SSID, String &pwd)
{
    prefs.begin("wifi");
    SSID = prefs.getString("SSID");
    pwd = prefs.getString("pwd");
    prefs.end();
}

void saveSystemCfg()
{
    prefs.begin("system");
    prefs.putBytes("general",&generalCfg, sizeof(generalCfg));
    for (int i=0;i<Units.size();i++)
    {
        const auto &cfg = Units[i].cfg;
        prefs.putBytes((String("unit")+String(i)).c_str(),(void*)&cfg, sizeof(UnitConfig));
        Units[i].plug.setCredentials(cfg.plugAddr,cfg.plugPwd);        
    }
    prefs.end();    

    LOG("System config is saved");    
}

void loadSystemCfg()
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
        cfg.plugPwd[sizeof(cfg.plugPwd) - 1] = 0;

        if (cfg.minThr < 0 || cfg.minThr > 100)
        {
            cfg.minThr = 0;
        }
        if (cfg.maxThr < 0 || cfg.maxThr > 100)
        {
            cfg.maxThr = 0;
        }

        Units[i].plug.setCredentials(cfg.plugAddr,cfg.plugPwd);
    }
    prefs.end();
}

