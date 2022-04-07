#include "Config.h"

Preferences prefs;
GeneralConfig generalCfg;

void saveWifiCfg(String SSID, String pwd)
{
    if (prefs.begin("wifi"))
    {
        prefs.putString("SSID", SSID);
        prefs.putString("pwd", pwd);
        prefs.end();
    }
}

void loadWifiCfg(String &SSID, String &pwd)
{
    if (prefs.begin("wifi", true))
    {
        SSID = prefs.getString("SSID");
        pwd = prefs.getString("pwd");
        prefs.end();
    }
}

bool saveSystemCfg()
{
    bool retVal=false;
    if (prefs.begin("system"))
    {
        if (prefs.putBytes("general", &generalCfg, sizeof(generalCfg)))
        {
            for (int i = 0; i < Units.size(); i++)
            {
                const auto &cfg = Units[i].cfg;
                if (prefs.putBytes((String("unit") + String(i)).c_str(), (void *)&cfg, sizeof(UnitConfig)))
                {
                    Units[i].plug.setCredentials(cfg.plugAddr, cfg.plugPwd);
                }
                else
                {
                    LOGERR("Cannot save config for unit %d", i);
                    break;
                }
            }
        }
        else
        {
            LOGERR("Cannot save system config");
        }
        prefs.end();

        retVal=true;
    }
    return retVal;
}

bool loadSystemCfg()
{
    bool retVal=false;
    if (prefs.begin("system", true))
    {
        if (prefs.getBytes("general", &generalCfg, sizeof(generalCfg)))
        {

            if (generalCfg.pollInterval < 0 || generalCfg.pollInterval > 32000)
            {
                generalCfg.pollInterval = 30;
            }

            if (generalCfg.UTCOffset < -23 || generalCfg.UTCOffset > 23)
            {
                generalCfg.pollInterval = 30;
            }
            generalCfg.NTPServer[sizeof(generalCfg.NTPServer) - 1] = 0;

            for (int i = 0; i < Units.size(); i++)
            {
                auto &cfg = Units[i].cfg;
                cfg = UnitConfig();
                if (prefs.getBytes((String("unit") + String(i)).c_str(), &cfg, sizeof(UnitConfig)))
                {
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
                    if (cfg.enStartTime >= 24 * 3600)
                    {
                        cfg.enStartTime = 0;
                    }
                    if (cfg.enEndTime >= 24 * 3600)
                    {
                        cfg.enEndTime = 0;
                    }

                    Units[i].plug.setCredentials(cfg.plugAddr, cfg.plugPwd);
                }
                else
                {
                    LOGERR("Cannot load config for unit %d", i);
                    break;
                }
            }
            retVal=true;                                
        }
        else
        {
            LOGERR("Cannot load general system config");
        }
        prefs.end();
    }
    return retVal;
}
