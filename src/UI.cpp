// This is the main function which builds our GUI
#include "Config.h"
#include "UI.h"

GeneralCfgIDs generalCfgIDs;

String ledStyle = "<span style='border-radius:50%;width:15px;height:15px;display:inline-block;border:1px;border-style:solid;margin-right:5px;color:dimgray;background:";
String divStyle = "<div style='width:7em;display:inline-block'>";
#define LED_LABEL(color, text) ledStyle + #color "'/>" + divStyle + #text "</span>"

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

        ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Min Humidity : ", None, root), clearLabelStyle);
        Units[i].IDs.idMin = ESPUI.addControl(Number, "", String(Units[i].cfg.minThr), None, root, generalCallback);
        ESPUI.addControl(Min, "", "0", None, Units[i].IDs.idMin);
        ESPUI.addControl(Max, "", "100", None, Units[i].IDs.idMin);

        ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Max Humidity : ", None, root), clearLabelStyle);
        Units[i].IDs.idMax = ESPUI.addControl(Number, "", String(Units[i].cfg.maxThr), None, root, generalCallback);
        ESPUI.addControl(Min, "", "0", None, Units[i].IDs.idMax);
        ESPUI.addControl(Max, "", "100", None, Units[i].IDs.idMax);

        ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Enabled Time Start : ", None, root), clearLabelStyle);
        Units[i].IDs.idEnStartTime = ESPUI.addControl(Text, "", String(Units[i].cfg.getEnStartTime()), None, root, generalCallback);

        ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Enabled Time End : ", None, root), clearLabelStyle);
        Units[i].IDs.idEnEndTime = ESPUI.addControl(Text, "", String(Units[i].cfg.getEnStartTime()), None, root, generalCallback);

        ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Smart Plug : ", None, root), clearLabelStyle);
        Units[i].IDs.idPlugAddr = ESPUI.addControl(Text, "", Units[i].cfg.plugAddr, None, root, textCallback);
        ESPUI.addControl(Max, "", "127", None, Units[i].IDs.idPlugAddr);

        ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Plug Pwd: ", None, root), clearLabelStyle);
        Units[i].IDs.idPlugPwd = ESPUI.addControl(Text, "", Units[i].cfg.plugPwd, None, root, textCallback);
        ESPUI.addControl(Max, "", "31", None, Units[i].IDs.idPlugPwd);
    }

    generalCfgIDs.idPollInterval = ESPUI.addControl(Number, "Poll Interval", String(generalCfg.pollInterval), None, settingsTab, textCallback);
    ESPUI.addControl(Min, "", "10", None, generalCfgIDs.idPollInterval);
    ESPUI.addControl(Max, "", "32000", None, generalCfgIDs.idPollInterval);

    generalCfgIDs.idSave = ESPUI.addControl(Button, "Save / Reset", "Save", Peterriver, settingsTab, saveCfgCallback);
    generalCfgIDs.idReset = ESPUI.addControl(Button, "", "Reset", Peterriver, generalCfgIDs.idSave, resetCfgCallback);

    ESPUI.addControl(Separator, "Found Sensors", "", None, settingsTab);
    generalCfgIDs.idFoundSensors = ESPUI.addControl(Label, "Found Sensors", getFoundSensorsList(), Turquoise, settingsTab, generalCallback);
    generalCfgIDs.idRefresh = ESPUI.addControl(Button, "", "Refresh", Turquoise, generalCfgIDs.idFoundSensors, refreshSensorsListCallback);

    /*
     * Tab: WiFi Credentials
     * You use this tab to enter the SSID and password of a wifi network to autoconnect to.
     *-----------------------------------------------------------------------------------------------------------*/
    auto wifitab = ESPUI.addControl(Tab, "", "WiFi Credentials");
    generalCfgIDs.idSSID = ESPUI.addControl(Text, "SSID", "", Alizarin, wifitab, textCallback);
    // Note that adding a "Max" control to a text control sets the max length
    ESPUI.addControl(Max, "", "32", None, generalCfgIDs.idSSID);
    generalCfgIDs.idPass = ESPUI.addControl(Text, "Password", "", Alizarin, wifitab, textCallback);
    ESPUI.addControl(Max, "", "64", None, generalCfgIDs.idPass);
    ESPUI.addControl(Button, "Save", "Save", Peterriver, wifitab, enterWifiDetailsCallback);

    // Finally, start up the UI.
    // This should only be called once we are connected to WiFi.
    ESPUI.begin("Humidity Control");
}

void generalCallback(Control *sender, int type)
{
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
        ESPUI.updateControlValue(Units[i].IDs.idEnStartTime, String(Units[i].cfg.getEnStartTime()));
        ESPUI.updateControlValue(Units[i].IDs.idEnEndTime, String(Units[i].cfg.getEnEndTime()));
        ESPUI.updateControlValue(Units[i].IDs.idPlugAddr, Units[i].cfg.plugAddr);
        ESPUI.updateControlValue(Units[i].IDs.idPlugPwd, Units[i].cfg.plugPwd);
    }
    ESPUI.updateControlValue(generalCfgIDs.idPollInterval, String(generalCfg.pollInterval));
}

void updateReadingsGUI(const std::map<std::string, SensorData> &readings)
{
    for (int i = 0; i < Units.size(); i++)
    {

        try
        {
            const auto &data = readings.at(Units[i].cfg.addr);
            ESPUI.updateLabel(Units[i].IDs.idLabel, String((int)data.humidity) + "%    " + String(data.temp, 1) + "&#176C    (" +
                                                        String(data.voltage) + " V)");
        }
        catch (...)
        {
            ESPUI.updateLabel(Units[i].IDs.idLabel, "- NO DATA -");
        }

        try
        {
            const auto plugReadings = Units[i].plug.getReadings();
            if (plugReadings.isEmpty())
            {
                throw std::runtime_error("no data");
            }

            if (plugReadings.isOn())
            {
                ESPUI.updateLabel(Units[i].IDs.idOn, LED_LABEL(green, Spraying));
                ESPUI.updateLabel(Units[i].IDs.idWater, plugReadings.isLoaded() ? LED_LABEL(blue, Water) : LED_LABEL(red, No Water));
            }
            else
            {
                ESPUI.updateLabel(Units[i].IDs.idOn, LED_LABEL(black, Off));
                ESPUI.updateLabel(Units[i].IDs.idWater, LED_LABEL(gray, ---));
            }
        }
        catch (...)
        {
            ESPUI.updateLabel(Units[i].IDs.idOn, LED_LABEL(gray, Unknown));
            ESPUI.updateLabel(Units[i].IDs.idWater, LED_LABEL(gray, Unknown));
        }
    }
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
        saveWifiCfg(ESPUI.getControl(generalCfgIDs.idSSID)->value, ESPUI.getControl(generalCfgIDs.idPass)->value);
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
            Units[i].cfg.setEnStartTime(ESPUI.getControl(Units[i].IDs.idEnStartTime)->value);
            Units[i].cfg.setEnEndTime(ESPUI.getControl(Units[i].IDs.idEnEndTime)->value);
            strncpy(Units[i].cfg.plugAddr, ESPUI.getControl(Units[i].IDs.idPlugAddr)->value.c_str(), sizeof(Units[i].cfg.plugAddr));
            Units[i].cfg.plugAddr[sizeof(Units[i].cfg.plugAddr) - 1] = 0;
            strncpy(Units[i].cfg.plugPwd, ESPUI.getControl(Units[i].IDs.idPlugPwd)->value.c_str(), sizeof(Units[i].cfg.plugPwd));
            Units[i].cfg.plugPwd[sizeof(Units[i].cfg.plugPwd) - 1] = 0;
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
