#include "WebServer.h"

#include <AsyncJson.h>

#include "ElegooCC.h"
#include "Logger.h"

#define SPIFFS LittleFS

// External reference to firmware version from main.cpp
extern const char *firmwareVersion;
extern const char *chipFamily;

WebServer::WebServer(int port) : server(port) {}

void WebServer::begin()
{
    server.begin();

    // Get settings endpoint
    server.on("/get_settings", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String jsonResponse = settingsManager.toJson(false);
                  request->send(200, "application/json", jsonResponse);
              });

    server.addHandler(new AsyncCallbackJsonWebHandler(
        "/update_settings",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
        {
            JsonObject jsonObj = json.as<JsonObject>();
            settingsManager.setElegooIP(jsonObj["elegooip"].as<String>());
            settingsManager.setSSID(jsonObj["ssid"].as<String>());
            settingsManager.setElegooIP(jsonObj["elegooip"].as<String>());
            settingsManager.setSSID(jsonObj["ssid"].as<String>());
            if (jsonObj.containsKey("passwd") && jsonObj["passwd"].as<String>().length() > 0)
            {
                settingsManager.setPassword(jsonObj["passwd"].as<String>());
            }
            settingsManager.setAPMode(jsonObj["ap_mode"].as<bool>());
            settingsManager.setTimeout(jsonObj["timeout"].as<int>());
            settingsManager.setPauseOnRunout(jsonObj["pause_on_runout"].as<bool>());
            settingsManager.setEnabled(jsonObj["enabled"].as<bool>());
            settingsManager.save();
            jsonObj.clear();
            request->send(200, "text/plain", "ok");
        }));

    // Setup ElegantOTA
    ElegantOTA.begin(&server);

    // Sensor status endpoint
    server.on("/sensor_status", HTTP_GET,
              [this](AsyncWebServerRequest *request)
              {
                  // Add elegoo status information using singleton
                  printer_info_t elegooStatus = elegooCC.getCurrentInformation();

                  DynamicJsonDocument jsonDoc(512);
                  jsonDoc["stopped"]        = elegooStatus.filamentStopped;
                  jsonDoc["filamentRunout"] = elegooStatus.filamentRunout;

                  jsonDoc["elegoo"]["mainboardID"]          = elegooStatus.mainboardID;
                  jsonDoc["elegoo"]["printStatus"]          = (int) elegooStatus.printStatus;
                  jsonDoc["elegoo"]["isPrinting"]           = elegooStatus.isPrinting;
                  jsonDoc["elegoo"]["currentLayer"]         = elegooStatus.currentLayer;
                  jsonDoc["elegoo"]["totalLayer"]           = elegooStatus.totalLayer;
                  jsonDoc["elegoo"]["progress"]             = elegooStatus.progress;
                  jsonDoc["elegoo"]["currentTicks"]         = elegooStatus.currentTicks;
                  jsonDoc["elegoo"]["totalTicks"]           = elegooStatus.totalTicks;
                  jsonDoc["elegoo"]["PrintSpeedPct"]        = elegooStatus.PrintSpeedPct;
                  jsonDoc["elegoo"]["isWebsocketConnected"] = elegooStatus.isWebsocketConnected;
                  jsonDoc["elegoo"]["currentZ"]             = elegooStatus.currentZ;

                  String jsonResponse;
                  serializeJson(jsonDoc, jsonResponse);
                  request->send(200, "application/json", jsonResponse);
              });

    // Logs endpoint
    server.on("/logs", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String jsonResponse = logger.getLogsAsJson();
                  request->send(200, "application/json", jsonResponse);
              });

    // Version endpoint
    server.on("/version", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  DynamicJsonDocument jsonDoc(256);
                  jsonDoc["firmware_version"] = firmwareVersion;
                  jsonDoc["chip_family"]      = chipFamily;
                  jsonDoc["build_date"]       = __DATE__;
                  jsonDoc["build_time"]       = __TIME__;

                  String jsonResponse;
                  serializeJson(jsonDoc, jsonResponse);
                  request->send(200, "application/json", jsonResponse);
              });

    // Serve static files from SPIFFS
    server.serveStatic("/assets/", SPIFFS, "/assets/");
    server.serveStatic("/", SPIFFS, "/");
}

void WebServer::loop()
{
    ElegantOTA.loop();
}