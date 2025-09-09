#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "ElegooCC.h"
#include "LittleFS.h"
#include "Logger.h"
#include "SettingsManager.h"
#include "WebServer.h"
#include "time.h"

#define SPIFFS LittleFS

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifndef FIRMWARE_VERSION_RAW
#define FIRMWARE_VERSION_RAW dev
#endif
#ifndef CHIP_FAMILY_RAW
#define CHIP_FAMILY_RAW Unknown
#endif

const char* firmwareVersion = TOSTRING(FIRMWARE_VERSION_RAW);
const char* chipFamily      = TOSTRING(CHIP_FAMILY_RAW);

#define WIFI_CHECK_INTERVAL 30000     // Check WiFi every 30 seconds
#define WIFI_RECONNECT_TIMEOUT 10000  // Wait 10 seconds for reconnection
#define NTP_SYNC_INTERVAL 3600000     // Re-sync with NTP every hour (3600000 ms)

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

WebServer webServer(80);

// Variables to track WiFi connection monitoring
unsigned long lastWifiCheck      = 0;
unsigned long wifiReconnectStart = 0;
bool          isReconnecting     = false;

// Variables to track NTP synchronization
unsigned long lastNTPSync = 0;

// If wifi fails, revert to AP mode and restart (only if never connected before);
void failWifi()
{
    // Only revert to AP mode if WiFi has never successfully connected
    if (!settingsManager.getHasConnected())
    {
        settingsManager.setAPMode(true);
        if (settingsManager.save())
        {
            logger.log("Failed to connect to wifi, reverted to AP mode (first connection attempt)");
        }
        else
        {
            logger.log("Failed to update settings");
        }

        delay(1000);  // Give time for serial output
        ESP.restart();
    }
    else
    {
        logger.log("WiFi connection failed, retrying in 30 seconds");
        // Don't restart, just continue trying to reconnect in checkWifiConnection()
    }
}

void wifiSetup()
{
    if (settingsManager.isAPMode())
    {
        logger.log("Starting AP mode");
        WiFi.softAP("ElegooXBTTSFS20", "elegooccsfs20");
    }
    else
    {
        logger.logf("Connecting to WiFi: %s", settingsManager.getSSID().c_str());
        WiFi.begin(settingsManager.getSSID().c_str(), settingsManager.getPassword().c_str());
        int limit = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.print('.');
            delay(1000);
            limit++;
            if (limit > 30)
            {
                failWifi();
                break;
            }
        }

        Serial.println();
        logger.log("WiFi Connected");

        // Mark that WiFi has successfully connected at least once
        if (!settingsManager.getHasConnected())
        {
            settingsManager.setHasConnected(true);
            settingsManager.save();
            logger.log("First successful WiFi connection recorded");
        }

        if (!MDNS.begin("ccxsfs20"))
        {
            logger.log("Error setting up MDNS responder!");
        }
    }
}

void checkWifiConnection()
{
    logger.log("Checking WiFi connection");

    // Skip check if already in AP mode
    if (settingsManager.isAPMode())
    {
        logger.log("Skipping WiFi check in AP mode");
        return;
    }

    // Check if WiFi is connected
    if (WiFi.status() != WL_CONNECTED)
    {
        if (!isReconnecting)
        {
            logger.log("WiFi disconnected, attempting to reconnect...");
            WiFi.begin(settingsManager.getSSID().c_str(), settingsManager.getPassword().c_str());
            wifiReconnectStart = millis();
            isReconnecting     = true;
        }
        else
        {
            // Check if reconnection timeout has elapsed
            if (millis() - wifiReconnectStart >= WIFI_RECONNECT_TIMEOUT)
            {
                failWifi();
            }
        }
    }
    else
    {
        // WiFi is connected, reset reconnection state
        if (isReconnecting)
        {
            logger.log("WiFi reconnected successfully");
            isReconnecting = false;

            // Mark that WiFi has successfully connected at least once
            if (!settingsManager.getHasConnected())
            {
                settingsManager.setHasConnected(true);
                settingsManager.save();
            }
        }
    }
}

void setup()
{
    // put your setup code here, to run once:
    pinMode(FILAMENT_RUNOUT_PIN, INPUT_PULLUP);
    pinMode(MOVEMENT_SENSOR_PIN, INPUT_PULLUP);
    Serial.begin(115200);

    // Initialize logging system
    logger.log("ESP SFS System starting up...");
    logger.logf("Firmware version: %s", firmwareVersion);
    logger.logf("Chip family: %s", chipFamily);

    SPIFFS.begin();  // note: this must be done before wifi/server setup
    logger.log("LittleFS filesystem initialized");

    // Load settings early
    settingsManager.load();

    wifiSetup();
    logger.log("NTP time synchronization started");
    configTime(0, 0, ntpServer);
    webServer.begin();
    logger.log("Web server started");
    elegooCC.setup();
    logger.log("ElegooCC communication initialized");

    logger.log("System initialization complete");
}

void syncTimeWithNTP()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        logger.log("NTP time synchronization successful");
    }
    else
    {
        logger.log("NTP time synchronization failed");
    }
}

unsigned long getTime()
{
    time_t now;
    time(&now);
    return now;
}

void loop()
{
    unsigned long currentTime     = millis();
    bool          isWifiConnected = !settingsManager.isAPMode() && WiFi.status() == WL_CONNECTED;

    // Check if restart is requested
    if (settingsManager.requestRestartAt > 0 && currentTime >= settingsManager.requestRestartAt)
    {
        logger.log("Restarting device due to WiFi settings change...");
        ESP.restart();
    }

    if (isWifiConnected)
    {
        elegooCC.loop();

        // Periodic NTP synchronization
        if (currentTime - lastNTPSync >= NTP_SYNC_INTERVAL)
        {
            lastNTPSync = currentTime;
            syncTimeWithNTP();
        }
    }
    else if (currentTime - lastWifiCheck >= WIFI_CHECK_INTERVAL)
    {
        checkWifiConnection();
    }

    webServer.loop();
}
