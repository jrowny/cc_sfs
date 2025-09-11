#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "ElegooCC.h"
#include "LittleFS.h"
#include "Logger.h"
#include "SettingsManager.h"
#include "WebServer.h"
#include "improv.h"
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

// These things get setup in the loop, not setup, so we need to track if they've happened
bool isWifiSetup      = false;
bool isElegooSetup    = false;
bool isWebServerSetup = false;
bool isNtpSetup       = false;

// Used by improv-wifi to parse serial data
uint8_t x_buffer[16];
uint8_t x_position = 0;

// Variables to track NTP synchronization
unsigned long lastNTPSyncAttempt = 0;

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

void startAPMode()
{
    logger.log("Starting AP mode");
    WiFi.softAP("ElegooXBTTSFS20", "elegooccsfs20");
    // Stop mDNS as it's not needed in AP mode
    MDNS.end();
}

void handleSuccessfulWifiConnection()
{
    logger.log("WiFi Connected");

    // Mark that WiFi has successfully connected at least once
    if (!settingsManager.getHasConnected())
    {
        settingsManager.setHasConnected(true);
        settingsManager.save();
        logger.log("First successful WiFi connection recorded");
    }

    // Reset any reconnection state
    isReconnecting = false;

    // Start/restart mDNS for station mode
    MDNS.end();
    if (!MDNS.begin("ccxsfs20"))
    {
        logger.log("Error setting up MDNS responder!");
    }
}

bool connectToWifiStation(bool isReconnect = false)
{
    const char* action = isReconnect ? "Reconnecting to" : "Connecting to";
    logger.logf("%s WiFi: %s", action, settingsManager.getSSID().c_str());

    WiFi.begin(settingsManager.getSSID().c_str(), settingsManager.getPassword().c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        Serial.print('.');
        delay(1000);
        attempts++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        handleSuccessfulWifiConnection();
        return true;
    }
    else
    {
        if (isReconnect)
        {
            logger.log("Failed to connect with new WiFi credentials");
        }
        else
        {
            failWifi();
        }
        return false;
    }
}

void cleanupWifiConnections()
{
    // Stop AP mode if it was running
    WiFi.softAPdisconnect(true);
    // Disconnect from any existing station connection
    WiFi.disconnect(true);
    delay(1000);
}

bool wifiSetup()
{
    if (settingsManager.isAPMode())
    {
        startAPMode();
        logger.log("Wifi setup in AP mode");
        return false;
    }
    else
    {
        return connectToWifiStation(false);
    }
}

bool reconnectWifiWithNewCredentials()
{
    logger.log("Applying new WiFi credentials...");

    // Clean up any existing connections first
    cleanupWifiConnections();

    // Check if we're switching to AP mode
    if (settingsManager.isAPMode())
    {
        logger.log("Switching to AP mode");
        startAPMode();
        return false;
    }

    // We're switching to or staying in station mode
    logger.log("Connecting to WiFi station mode with new credentials...");
    return connectToWifiStation(true);
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
    logger.log("Filesystem initialized");

    // Load settings early
    settingsManager.load();
    logger.log("Settings Manager Loaded");
}

void syncTimeWithNTP(unsigned long currentTime)
{
    struct tm timeinfo;
    lastNTPSyncAttempt = currentTime;
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

void onImprovErrorCallback(improv::Error err)
{
    // log it?
    logger.logf("Improv error: %d", err);
}

void set_state(improv::State state)
{
    std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
    data.resize(11);
    data[6] = improv::IMPROV_SERIAL_VERSION;
    data[7] = improv::TYPE_CURRENT_STATE;
    data[8] = 1;
    data[9] = state;

    uint8_t checksum = 0x00;
    for (uint8_t d : data)
        checksum += d;
    data[10] = checksum;

    Serial.write(data.data(), data.size());
}

void send_response(std::vector<uint8_t>& response)
{
    std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
    data.resize(9);
    data[6] = improv::IMPROV_SERIAL_VERSION;
    data[7] = improv::TYPE_RPC_RESPONSE;
    data[8] = response.size();
    data.insert(data.end(), response.begin(), response.end());

    uint8_t checksum = 0x00;
    for (uint8_t d : data)
        checksum += d;
    data.push_back(checksum);

    Serial.write(data.data(), data.size());
}

void set_error(improv::Error error)
{
    std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
    data.resize(11);
    data[6] = improv::IMPROV_SERIAL_VERSION;
    data[7] = improv::TYPE_ERROR_STATE;
    data[8] = 1;
    data[9] = error;

    uint8_t checksum = 0x00;
    for (uint8_t d : data)
        checksum += d;
    data[10] = checksum;

    Serial.write(data.data(), data.size());
}
std::vector<std::string> getLocalUrl()
{
    return {// URL where user can finish onboarding or use device
            // Recommended to use website hosted by device
            String("http://" + WiFi.localIP().toString()).c_str()};
}

void getAvailableWifiNetworks()
{
    int networkNum = WiFi.scanNetworks();

    for (int id = 0; id < networkNum; ++id)
    {
        std::vector<uint8_t> data =
            improv::build_rpc_response(improv::GET_WIFI_NETWORKS,
                                       {WiFi.SSID(id), String(WiFi.RSSI(id)),
                                        (WiFi.encryptionType(id) == WIFI_AUTH_OPEN ? "NO" : "YES")},
                                       false);
        send_response(data);
        delay(1);
    }
    // final response
    std::vector<uint8_t> data =
        improv::build_rpc_response(improv::GET_WIFI_NETWORKS, std::vector<std::string>{}, false);
    send_response(data);
}

bool onImprovCommandCallback(improv::ImprovCommand cmd)
{
    switch (cmd.command)
    {
        case improv::Command::GET_CURRENT_STATE:
        {
            if ((WiFi.status() == WL_CONNECTED))
            {
                set_state(improv::State::STATE_PROVISIONED);
                std::vector<uint8_t> data =
                    improv::build_rpc_response(improv::GET_CURRENT_STATE, getLocalUrl(), false);
                send_response(data);
            }
            else
            {
                set_state(improv::State::STATE_AUTHORIZED);
            }

            break;
        }

        case improv::Command::WIFI_SETTINGS:
        {
            if (cmd.ssid.length() == 0)
            {
                set_error(improv::Error::ERROR_INVALID_RPC);
                break;
            }

            set_state(improv::STATE_PROVISIONING);

            settingsManager.setSSID(cmd.ssid.c_str());
            settingsManager.setPassword(cmd.password.c_str());
            settingsManager.setAPMode(false);
            settingsManager.save(true);  // skip wifi check, we're about to try connecting

            if (reconnectWifiWithNewCredentials())  // connectWifi(cmd.ssid, cmd.password)
            {
                set_state(improv::STATE_PROVISIONED);
                std::vector<uint8_t> data =
                    improv::build_rpc_response(improv::WIFI_SETTINGS, getLocalUrl(), false);
                send_response(data);
            }
            else
            {
                set_state(improv::STATE_STOPPED);
                set_error(improv::Error::ERROR_UNABLE_TO_CONNECT);
            }

            break;
        }

        case improv::Command::GET_DEVICE_INFO:
        {
            std::vector<std::string> infos = {// Firmware name
                                              "CC_SFS",
                                              // Firmware version
                                              firmwareVersion,
                                              // Hardware chip/variant
                                              chipFamily,
                                              // Device name
                                              "CC_SFS"};
            std::vector<uint8_t>     data =
                improv::build_rpc_response(improv::GET_DEVICE_INFO, infos, false);
            send_response(data);
            break;
        }

        case improv::Command::GET_WIFI_NETWORKS:
        {
            getAvailableWifiNetworks();
            break;
        }

        default:
        {
            set_error(improv::ERROR_UNKNOWN_RPC);
            return false;
        }
    }

    return true;
}

bool handleImprovWifi()
{
    if (Serial.available() > 0)
    {
        uint8_t b = Serial.read();

        if (parse_improv_serial_byte(x_position, b, x_buffer, onImprovCommandCallback,
                                     onImprovErrorCallback))
        {
            x_buffer[x_position++] = b;
        }
        else
        {
            x_position = 0;
        }
        return true;
    }
    return false;
}

void loop()
{
    // handling immprovWifi should be the first thing we do
    if (handleImprovWifi())
    {
        // if we handled serial data, don't return so we don't bother with the rest of the setup
        return;
    }
    unsigned long currentTime     = millis();
    bool          isWifiConnected = !settingsManager.isAPMode() && WiFi.status() == WL_CONNECTED;

    if (!isWifiSetup)
    {
        isWifiSetup = wifiSetup();
        isWifiSetup = true;
        logger.log("Wifi setup complete");
        return;  //
    }
    if (!isWebServerSetup)
    {
        webServer.begin();
        isWebServerSetup = true;
        logger.log("Webserver setup complete");
        return;  //
    }

    // Check if WiFi reconnection is requested

    if (settingsManager.requestWifiReconnect)
    {
        settingsManager.requestWifiReconnect = false;
        reconnectWifiWithNewCredentials();
    }

    if (isWifiConnected)
    {
        if (!isElegooSetup)
        {
            elegooCC.setup();
            logger.log("Elegoo setup complete");
            isElegooSetup = true;
        }
        elegooCC.loop();

        if (!isNtpSetup)
        {
            configTime(0, 0, ntpServer);
            syncTimeWithNTP(currentTime);
            logger.log("NTP setup complete");
            isNtpSetup = true;
        }
        else if (currentTime - lastNTPSyncAttempt >= NTP_SYNC_INTERVAL)
        {
            syncTimeWithNTP(currentTime);
        }
    }
    else if (currentTime - lastWifiCheck >= WIFI_CHECK_INTERVAL)
    {
        checkWifiConnection();
    }

    webServer.loop();
}
