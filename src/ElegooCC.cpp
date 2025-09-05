#include "ElegooCC.h"

#include <ArduinoJson.h>

#include "Logger.h"
#include "SettingsManager.h"

#define ACK_TIMEOUT_MS 5000

// External function to get current time (from main.cpp)
extern unsigned long getTime();

ElegooCC &ElegooCC::getInstance()
{
    static ElegooCC instance;
    return instance;
}

ElegooCC::ElegooCC()
{
    lastMovementValue = -1;
    lastChangeTime    = 0;

    mainboardID       = "";
    printStatus       = SDCP_PRINT_STATUS_IDLE;
    machineStatusMask = 0;  // No statuses active initially
    currentLayer      = 0;
    totalLayer        = 0;
    progress          = 0;
    currentTicks      = 0;
    totalTicks        = 0;
    PrintSpeedPct     = 0;
    filamentStopped   = false;
    filamentRunout    = false;
    lastPing          = 0;

    waitingForAck       = false;
    pendingAckCommand   = -1;
    pendingAckRequestId = "";
    ackWaitStartTime    = 0;

    // TODO: send a UDP broadcast, M99999 on Port 30000, maybe using AsyncUDP.h and listen for the
    // result. this will give us the printer IP address.

    // event handler - use lambda to capture 'this' pointer
    webSocket.onEvent([this](WStype_t type, uint8_t *payload, size_t length)
                      { this->webSocketEvent(type, payload, length); });
}

void ElegooCC::setup()
{
    bool shouldConect = !settingsManager.isAPMode();
    if (shouldConect)
    {
        connect();
    }
}

void ElegooCC::webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
        case WStype_DISCONNECTED:
            logger.log("Disconnected from Carbon Centauri");
            // Reset acknowledgment state on disconnect
            waitingForAck       = false;
            pendingAckCommand   = -1;
            pendingAckRequestId = "";
            ackWaitStartTime    = 0;
            break;
        case WStype_CONNECTED:
            logger.log("Connected to Carbon Centauri");
            sendCommand(SDCP_COMMAND_STATUS);

            break;
        case WStype_TEXT:
        {
            StaticJsonDocument<2048> doc;
            DeserializationError     error = deserializeJson(doc, payload);

            if (error)
            {
                logger.logf("JSON parsing failed: %s", error.c_str());
                return;
            }

            // Check if this is a command acknowledgment response
            if (doc.containsKey("Id") && doc.containsKey("Data"))
            {
                handleCommandResponse(doc);
            }
            // Check if this is a status response
            else if (doc.containsKey("Status"))
            {
                handleStatus(doc);
            }
        }
        break;
        case WStype_BIN:
            logger.log("Received unspported binary data");
            break;
        case WStype_ERROR:
            logger.logf("WebSocket error: %s", payload);
            break;
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            logger.log("Received unspported fragment data");
            break;
    }
}

void ElegooCC::handleCommandResponse(JsonDocument &doc)
{
    String     id   = doc["Id"];
    JsonObject data = doc["Data"];

    if (data.containsKey("Cmd") && data.containsKey("RequestID"))
    {
        int    cmd         = data["Cmd"];
        int    ack         = data["Data"]["Ack"];
        String requestId   = data["RequestID"];
        String mainboardId = data["MainboardID"];

        logger.logf("Command %d acknowledged (Ack: %d) for request %s", cmd, ack,
                    requestId.c_str());

        // Check if this is the acknowledgment we're waiting for
        if (waitingForAck && cmd == pendingAckCommand && requestId == pendingAckRequestId)
        {
            logger.logf("Received expected acknowledgment for command %d", cmd);
            waitingForAck       = false;
            pendingAckCommand   = -1;
            pendingAckRequestId = "";
            ackWaitStartTime    = 0;
        }

        // Store mainboard ID if we don't have it yet
        if (mainboardID.isEmpty() && !mainboardId.isEmpty())
        {
            mainboardID = mainboardId;
            logger.logf("Stored MainboardID: %s", mainboardID.c_str());
        }
    }
}

void ElegooCC::handleStatus(JsonDocument &doc)
{
    JsonObject status      = doc["Status"];
    String     mainboardId = doc["MainboardID"];

    logger.log("Received status update:");

    // Parse current status (which contains machine status array)
    if (status.containsKey("CurrentStatus"))
    {
        JsonArray currentStatus = status["CurrentStatus"];

        // Convert JsonArray to int array for machine statuses
        int statuses[5];  // Max 5 statuses
        int count = min((int) currentStatus.size(), 5);
        for (int i = 0; i < count; i++)
        {
            statuses[i] = currentStatus[i].as<int>();
        }

        // Set all machine statuses at once
        setMachineStatuses(statuses, count);
    }

    // Parse CurrentCoords to extract Z coordinate
    if (status.containsKey("CurrenCoord"))
    {
        String coordsStr   = status["CurrenCoord"].as<String>();
        int    firstComma  = coordsStr.indexOf(',');
        int    secondComma = coordsStr.indexOf(',', firstComma + 1);
        if (firstComma != -1 && secondComma != -1)
        {
            String zStr = coordsStr.substring(secondComma + 1);
            currentZ    = zStr.toFloat();
        }
    }

    // Parse print info
    if (status.containsKey("PrintInfo"))
    {
        JsonObject          printInfo = status["PrintInfo"];
        sdcp_print_status_t newStatus = printInfo["Status"].as<sdcp_print_status_t>();
        if (newStatus != printStatus && newStatus == SDCP_PRINT_STATUS_PRINTING)
        {
            logger.log("Print status changed to printing");
            startedAt = millis();
        }
        printStatus   = newStatus;
        currentLayer  = printInfo["CurrentLayer"];
        totalLayer    = printInfo["TotalLayer"];
        progress      = printInfo["Progress"];
        currentTicks  = printInfo["CurrentTicks"];
        totalTicks    = printInfo["TotalTicks"];
        PrintSpeedPct = printInfo["PrintSpeedPct"];
    }

    // Store mainboard ID if we don't have it yet (I'm unsure if we actually need this)
    if (mainboardID.isEmpty() && !mainboardId.isEmpty())
    {
        mainboardID = mainboardId;
        logger.logf("Stored MainboardID: %s", mainboardID.c_str());
    }
}

void ElegooCC::pausePrint()
{
    sendCommand(SDCP_COMMAND_PAUSE_PRINT, true);
}

void ElegooCC::continuePrint()
{
    sendCommand(SDCP_COMMAND_CONTINUE_PRINT, true);
}

void ElegooCC::sendCommand(int command, bool waitForAck)
{
    if (!webSocket.isConnected())
    {
        logger.logf("Can't send command, websocket not connected: %d", command);
        return;
    }

    // If this command requires an ack and we're already waiting for one, skip it
    if (waitForAck && waitingForAck)
    {
        logger.logf("Skipping command %d - already waiting for ack from command %d", command,
                    pendingAckCommand);
        return;
    }

    uuid.generate();
    String uuidStr = String(uuid.toCharArray());
    uuidStr.replace("-", "");  // RequestID doesn't want dashes

    // Get current timestamp
    unsigned long timestamp   = getTime();
    String        jsonPayload = "{";
    jsonPayload += "\"Id\":\"" + uuidStr + "\",";
    jsonPayload += "\"Data\":{";
    jsonPayload += "\"Cmd\":" + String(command) + ",";
    jsonPayload += "\"Data\":{},";
    jsonPayload += "\"RequestID\":\"" + uuidStr + "\",";
    jsonPayload += "\"MainboardID\":\"" + mainboardID + "\",";
    jsonPayload += "\"TimeStamp\":" + String(timestamp) + ",";
    jsonPayload += "\"From\":2";  // I don't know if this is used, but octoeverywhere sets theirs to
                                  // 0, and the web client sets it to 1, so we'll choose 2?
    jsonPayload += "}";
    jsonPayload += "}";

    // If this command requires an ack, set the tracking state
    if (waitForAck)
    {
        waitingForAck       = true;
        pendingAckCommand   = command;
        pendingAckRequestId = uuidStr;
        ackWaitStartTime    = millis();
        logger.logf("Waiting for acknowledgment for command %d with request ID %s", command,
                    uuidStr.c_str());
    }

    webSocket.sendTXT(jsonPayload);
}

void ElegooCC::connect()
{
    if (webSocket.isConnected())
    {
        webSocket.disconnect();
    }
    webSocket.setReconnectInterval(3000);
    ipAddress = settingsManager.getElegooIP();
    logger.logf("Connecting to %s", ipAddress.c_str());
    webSocket.begin(ipAddress, CARBON_CENTAURI_PORT, "/websocket");
}

void ElegooCC::loop()
{
    unsigned long currentTime = millis();

    // websocket IP changed, reconnect
    if (ipAddress != settingsManager.getElegooIP())
    {
        connect();  // this will reconnnect if already connected
    }

    if (webSocket.isConnected())
    {
        // Check for acknowledgment timeout (5 seconds)
        // TODO: need to check the actual requestId
        if (waitingForAck && (currentTime - ackWaitStartTime) >= ACK_TIMEOUT_MS)
        {
            logger.logf("Acknowledgment timeout for command %d, resetting ack state",
                        pendingAckCommand);
            waitingForAck       = false;
            pendingAckCommand   = -1;
            pendingAckRequestId = "";
            ackWaitStartTime    = 0;
        }
        else if (currentTime - lastPing > 29900)
        {
            logger.log("Sending Ping");
            // For all who venture to this line of code wondering why I didn't use sendPing(), it's
            // because for some reason that doesn't work. but this does!
            this->webSocket.sendTXT("ping");
            lastPing = currentTime;
        }
    }

    // Before determining if we should pause, check if the filament is moving or it ran out
    checkFilamentMovement(currentTime);
    checkFilamentRunout(currentTime);

    // Check if we should pause the print
    if (shouldPausePrint(currentTime))
    {
        logger.log("Pausing print, detected filament runout or stopped");
        pausePrint();
    }

    webSocket.loop();
}

void ElegooCC::checkFilamentRunout(unsigned long currentTime)
{
    // The signal output of the switch sensor is at low level when no filament is detected
    bool newFilamentRunout = digitalRead(FILAMENT_RUNOUT_PIN) == LOW;
    if (newFilamentRunout != filamentRunout)
    {
        logger.log(filamentRunout ? "Filament has run out" : "Filament has been detected");
    }
    filamentRunout = newFilamentRunout;
}

void ElegooCC::checkFilamentMovement(unsigned long currentTime)
{
    int currentMovementValue = digitalRead(MOVEMENT_SENSOR_PIN);

    // CurrentLayer is unreliable when using Orcaslicer 2.3.0, because it is missing some g-code,so
    // we use Z instead. , assuming first layer is at Z offset <  0.1
    int movementTimeout =
        currentZ < 0.1 ? settingsManager.getFirstLayerTimeout() : settingsManager.getTimeout();

    // Check if movement sensor value has changed, if the filament is moving, it should change every
    // so often when it changes, reset the timeout
    if (currentMovementValue != lastMovementValue)
    {
        if (filamentStopped)
        {
            logger.log("Filament movement started");
        }
        // Value changed, reset timer and flag
        lastMovementValue = currentMovementValue;
        lastChangeTime    = currentTime;
        filamentStopped   = false;
    }
    else
    {
        // Value hasn't changed, check if timeout has elapsed
        if ((currentTime - lastChangeTime) >= movementTimeout && !filamentStopped)
        {
            logger.logf("Filament movement stopped, last movement detected %dms ago",
                        currentTime - lastChangeTime);
            filamentStopped = true;  // Prevent repeated printing
        }
    }
}

bool ElegooCC::shouldPausePrint(unsigned long currentTime)
{
    // If pause function is completely disabled, always return false
    if (!settingsManager.getEnabled())
    {
        return false;
    }

    if (filamentRunout && !settingsManager.getPauseOnRunout())
    {
        // if pause on runout is disabled, and filament ran out, skip checking everything else
        // this should let the carbon take care of itself
        return false;
    }

    // Only puase if getPauseOnRunout is enabled and filement runsout or filamentStopped.
    bool pauseCondition = filamentRunout || filamentStopped;

    // Don't pause in the first X milliseconds (configurable in settings)
    // Don't pause if the websocket is not connected (we can't pause anyway if we're not connected)
    // Don't pause if we're waiting for an ack
    // Don't pause if we have less than 100t tickets left, the print is probably done
    // TODO: also add a buffer after pause because sometimes an ack comes before the update
    if (currentTime - startedAt < settingsManager.getStartPrintTimeout() ||
        !webSocket.isConnected() || waitingForAck || !isPrinting() ||
        (totalTicks - currentTicks) < 100 || !pauseCondition)
    {
        return false;
    }

    // log why we paused...
    logger.logf("Pause condition: %d", pauseCondition);
    logger.logf("Filament runout: %d", filamentRunout);
    logger.logf("Filament runout pause enabled: %d", settingsManager.getPauseOnRunout());
    logger.logf("Filament stopped: %d", filamentStopped);
    logger.logf("Time since print start %d", currentTime - startedAt);
    logger.logf("Is Machine status printing?: %d", hasMachineStatus(SDCP_MACHINE_STATUS_PRINTING));
    logger.logf("Print status: %d", printStatus);

    return true;
}

bool ElegooCC::isPrinting()
{
    return printStatus == SDCP_PRINT_STATUS_PRINTING &&
           hasMachineStatus(SDCP_MACHINE_STATUS_PRINTING);
}

// Helper methods for machine status bitmask
bool ElegooCC::hasMachineStatus(sdcp_machine_status_t status)
{
    return (machineStatusMask & (1 << status)) != 0;
}

void ElegooCC::setMachineStatuses(const int *statusArray, int arraySize)
{
    machineStatusMask = 0;  // Clear all statuses first
    for (int i = 0; i < arraySize; i++)
    {
        if (statusArray[i] >= 0 && statusArray[i] <= 4)
        {  // Validate range
            machineStatusMask |= (1 << statusArray[i]);
        }
    }
}

// Get current printer information
printer_info_t ElegooCC::getCurrentInformation()
{
    printer_info_t info;

    info.filamentStopped      = filamentStopped;
    info.filamentRunout       = filamentRunout;
    info.mainboardID          = mainboardID;
    info.printStatus          = printStatus;
    info.isPrinting           = isPrinting();
    info.currentLayer         = currentLayer;
    info.totalLayer           = totalLayer;
    info.progress             = progress;
    info.currentTicks         = currentTicks;
    info.totalTicks           = totalTicks;
    info.PrintSpeedPct        = PrintSpeedPct;
    info.isWebsocketConnected = webSocket.isConnected();
    info.currentZ             = currentZ;
    info.waitingForAck        = waitingForAck;

    return info;
}