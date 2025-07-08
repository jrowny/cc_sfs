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
#define WIFI_CHECK_INTERVAL 30000     // Check WiFi every 30 seconds
#define WIFI_RECONNECT_TIMEOUT 10000  // Wait 10 seconds for reconnection

// NTP server to request epoch time
const char *ntpServer = "pool.ntp.org";

WebServer webServer(80);

// Variables to track WiFi connection monitoring
unsigned long lastWifiCheck      = 0;
unsigned long wifiReconnectStart = 0;
bool          isReconnecting     = false;

// If wifi fails, revert to AP mode and restart;
void failWifi()
{
    settingsManager.setAPMode(true);
    if (settingsManager.save())
    {
        logger.log("Failed to connect to wifi, reverted to AP mode");
    }
    else
    {
        logger.log("Failed to update settings");
    }

    delay(1000);  // Give time for serial output
    ESP.restart();
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

unsigned long getTime()
{
    time_t    now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        // Serial.println("Failed to obtain time");
        return (0);
    }
    time(&now);
    return now;
}

void loop()
{
    // Check WiFi connection periodically
    unsigned long currentTime = millis();

    // Check if restart is requested
    if (settingsManager.requestRestartAt > 0 && currentTime >= settingsManager.requestRestartAt)
    {
        logger.log("Restarting device due to WiFi settings change...");
        ESP.restart();
    }
    if (currentTime - lastWifiCheck >= WIFI_CHECK_INTERVAL)
    {
        lastWifiCheck = currentTime;
        checkWifiConnection();
    }

    elegooCC.loop();
    webServer.loop();
}
