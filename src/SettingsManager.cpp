#include "SettingsManager.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <stdlib.h>

#include "Logger.h"

SettingsManager &SettingsManager::getInstance()
{
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager()
{
    isLoaded                 = false;
    settings.ap_mode         = false;
    settings.ssid            = "";
    settings.passwd          = "";
    settings.elegooip        = "";
    settings.timeout         = 2000;
    settings.pause_on_runout = true;
}

bool SettingsManager::load()
{
    File file = LittleFS.open("/user_settings.json", "r");
    if (!file)
    {
        logger.log("Settings file not found, using defaults");
        isLoaded = true;
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError     error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        logger.log("Settings JSON parsing error, using defaults");
        isLoaded = true;
        return false;
    }

    settings.ap_mode         = doc["ap_mode"] | false;
    settings.ssid            = doc["ssid"] | "";
    settings.passwd          = doc["passwd"] | "";
    settings.elegooip        = doc["elegooip"] | "";
    settings.timeout         = doc["timeout"] | 2000;
    settings.pause_on_runout = doc["pause_on_runout"] | true;

    isLoaded = true;
    return true;
}

bool SettingsManager::save()
{
    String output = toJson(true);

    File file = LittleFS.open("/user_settings.json", "w");
    if (!file)
    {
        logger.log("Failed to open settings file for writing");
        return false;
    }

    if (file.print(output) == 0)
    {
        logger.log("Failed to write settings to file");
        file.close();
        return false;
    }

    file.close();
    logger.log("Settings saved successfully");
    if (wifiChanged)
    {
        logger.log("Wifi changed, requesting restart");
        requestRestartAt = millis() + 3000;
        wifiChanged      = false;
    }
    return true;
}

const user_settings &SettingsManager::getSettings()
{
    if (!isLoaded)
    {
        load();
    }
    return settings;
}

String SettingsManager::getSSID()
{
    return getSettings().ssid;
}

String SettingsManager::getPassword()
{
    return getSettings().passwd;
}

bool SettingsManager::isAPMode()
{
    return getSettings().ap_mode;
}

String SettingsManager::getElegooIP()
{
    return getSettings().elegooip;
}

int SettingsManager::getTimeout()
{
    return getSettings().timeout;
}

bool SettingsManager::getPauseOnRunout()
{
    return getSettings().pause_on_runout;
}

void SettingsManager::setSSID(const String &ssid)
{
    if (!isLoaded)
        load();
    if (settings.ssid != ssid)
    {
        settings.ssid = ssid;
        wifiChanged   = true;
    }
}

void SettingsManager::setPassword(const String &password)
{
    if (!isLoaded)
        load();
    if (settings.passwd != password)
    {
        settings.passwd = password;
        wifiChanged     = true;
    }
}

void SettingsManager::setAPMode(bool apMode)
{
    if (!isLoaded)
        load();
    if (settings.ap_mode != apMode)
    {
        settings.ap_mode = apMode;
        wifiChanged      = true;
    }
}

void SettingsManager::setElegooIP(const String &ip)
{
    if (!isLoaded)
        load();
    settings.elegooip = ip;
}

void SettingsManager::setTimeout(int timeout)
{
    if (!isLoaded)
        load();
    settings.timeout = timeout;
}

void SettingsManager::setPauseOnRunout(bool pauseOnRunout)
{
    if (!isLoaded)
        load();
    settings.pause_on_runout = pauseOnRunout;
}

String SettingsManager::toJson(bool includePassword)
{
    String                   output;
    StaticJsonDocument<1024> doc;

    doc["ap_mode"]         = settings.ap_mode;
    doc["ssid"]            = settings.ssid;
    doc["elegooip"]        = settings.elegooip;
    doc["timeout"]         = settings.timeout;
    doc["pause_on_runout"] = settings.pause_on_runout;

    if (includePassword)
    {
        doc["passwd"] = settings.passwd;
    }

    serializeJson(doc, output);
    return output;
}
