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
    isLoaded                     = false;
    requestWifiReconnect         = false;
    wifiChanged                  = false;
    settings.ap_mode             = false;
    settings.ssid                = "";
    settings.passwd              = "";
    settings.elegooip            = "";
    settings.timeout             = 4000;
    settings.first_layer_timeout = 8000;
    settings.pause_on_runout     = true;
    settings.start_print_timeout = 10000;
    settings.enabled             = true;
    settings.has_connected       = false;
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

    settings.ap_mode             = doc["ap_mode"] | false;
    settings.ssid                = doc["ssid"] | "";
    settings.passwd              = doc["passwd"] | "";
    settings.elegooip            = doc["elegooip"] | "";
    settings.timeout             = doc["timeout"] | 4000;
    settings.first_layer_timeout = doc["first_layer_timeout"] | 8000;
    settings.pause_on_runout     = doc["pause_on_runout"] | true;
    settings.enabled             = doc["enabled"] | true;
    settings.start_print_timeout = doc["start_print_timeout"] | 10000;
    settings.has_connected       = doc["has_connected"] | false;

    isLoaded = true;
    return true;
}

bool SettingsManager::save(bool skipWifiCheck)
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
    if (!skipWifiCheck && wifiChanged)
    {
        logger.log("WiFi changed, requesting reconnection");
        requestWifiReconnect = true;
        wifiChanged          = false;
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

int SettingsManager::getFirstLayerTimeout()
{
    return getSettings().first_layer_timeout;
}

bool SettingsManager::getPauseOnRunout()
{
    return getSettings().pause_on_runout;
}

int SettingsManager::getStartPrintTimeout()
{
    return getSettings().start_print_timeout;
}

bool SettingsManager::getEnabled()
{
    return getSettings().enabled;
}

bool SettingsManager::getHasConnected()
{
    return getSettings().has_connected;
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

void SettingsManager::setFirstLayerTimeout(int timeout)
{
    if (!isLoaded)
        load();
    settings.first_layer_timeout = timeout;
}

void SettingsManager::setPauseOnRunout(bool pauseOnRunout)
{
    if (!isLoaded)
        load();
    settings.pause_on_runout = pauseOnRunout;
}

void SettingsManager::setStartPrintTimeout(int timeoutMs)
{
    if (!isLoaded)
        load();
    settings.start_print_timeout = timeoutMs;
}

void SettingsManager::setEnabled(bool enabled)
{
    if (!isLoaded)
        load();
    settings.enabled = enabled;
}

void SettingsManager::setHasConnected(bool hasConnected)
{
    if (!isLoaded)
        load();
    settings.has_connected = hasConnected;
}

String SettingsManager::toJson(bool includePassword)
{
    String                   output;
    StaticJsonDocument<1024> doc;

    doc["ap_mode"]             = settings.ap_mode;
    doc["ssid"]                = settings.ssid;
    doc["elegooip"]            = settings.elegooip;
    doc["timeout"]             = settings.timeout;
    doc["first_layer_timeout"] = settings.first_layer_timeout;
    doc["pause_on_runout"]     = settings.pause_on_runout;
    doc["start_print_timeout"] = settings.start_print_timeout;
    doc["enabled"]             = settings.enabled;
    doc["has_connected"]       = settings.has_connected;

    if (includePassword)
    {
        doc["passwd"] = settings.passwd;
    }

    serializeJson(doc, output);
    return output;
}
