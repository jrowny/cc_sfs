#include <Arduino.h>
#include <ArduinoJson.h>

#ifndef SETTINGS_DATA_H
#define SETTINGS_DATA_H

struct user_settings
{
    String ssid;
    String passwd;
    bool   ap_mode;
    String elegooip;
    int    timeout;
    int    first_layer_timeout;
    bool   pause_on_runout;
    int    start_print_timeout;
    bool   enabled;
    bool   has_connected;
};

class SettingsManager
{
   private:
    user_settings settings;
    bool          isLoaded;
    bool          wifiChanged;

    SettingsManager();

    SettingsManager(const SettingsManager &)            = delete;
    SettingsManager &operator=(const SettingsManager &) = delete;

   public:
    static SettingsManager &getInstance();

    // After saving wifi settings, requests restart, TODO: maybe use an event instead of this
    unsigned long requestRestartAt;

    bool load();
    bool save();

    //  (loads if not already loaded)
    const user_settings &getSettings();

    String getSSID();
    String getPassword();
    bool   isAPMode();
    String getElegooIP();
    int    getTimeout();
    int    getFirstLayerTimeout();
    bool   getPauseOnRunout();
    int    getStartPrintTimeout();
    bool   getEnabled();
    bool   getHasConnected();

    void setSSID(const String &ssid);
    void setPassword(const String &password);
    void setAPMode(bool apMode);
    void setElegooIP(const String &ip);
    void setTimeout(int timeout);
    void setFirstLayerTimeout(int timeout);
    void setPauseOnRunout(bool pauseOnRunout);
    void setStartPrintTimeout(int timeoutMs);
    void setEnabled(bool enabled);
    void setHasConnected(bool hasConnected);

    String toJson(bool includePassword = true);
};

#define settingsManager SettingsManager::getInstance()

#endif