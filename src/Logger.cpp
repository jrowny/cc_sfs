#include "Logger.h"
#include "time.h"

// External function to get current time (from main.cpp)
extern unsigned long getTime();

Logger &Logger::getInstance()
{
  static Logger instance;
  return instance;
}

Logger::Logger()
{
  currentIndex = 0;
  totalEntries = 0;
  uuidGenerator.generate();
}

void Logger::log(const String &message)
{
  // Print to serial first
  Serial.println(message);

  // Generate UUID for this log entry
  uuidGenerator.generate();
  String uuid = String(uuidGenerator.toCharArray());

  // Get current timestamp
  unsigned long timestamp = getTime();

  // Store in circular buffer
  logBuffer[currentIndex].uuid = uuid;
  logBuffer[currentIndex].timestamp = timestamp;
  logBuffer[currentIndex].message = message;

  // Update indices
  currentIndex = (currentIndex + 1) % MAX_LOG_ENTRIES;
  if (totalEntries < MAX_LOG_ENTRIES)
  {
    totalEntries++;
  }
}

void Logger::log(const char *message)
{
  log(String(message));
}

void Logger::logf(const char *format, ...)
{
  char buffer[512];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  log(String(buffer));
}

String Logger::getLogsAsJson()
{
  DynamicJsonDocument jsonDoc(8192); // Allocate enough space for logs
  JsonArray logsArray = jsonDoc.createNestedArray("logs");

  // If we have less than MAX_LOG_ENTRIES, start from 0
  // Otherwise, start from currentIndex (oldest entry)
  int startIndex = (totalEntries < MAX_LOG_ENTRIES) ? 0 : currentIndex;
  int count = totalEntries;

  for (int i = 0; i < count; i++)
  {
    int bufferIndex = (startIndex + i) % MAX_LOG_ENTRIES;

    JsonObject logEntry = logsArray.createNestedObject();
    logEntry["uuid"] = logBuffer[bufferIndex].uuid;
    logEntry["timestamp"] = logBuffer[bufferIndex].timestamp;
    logEntry["message"] = logBuffer[bufferIndex].message;
  }

  String jsonResponse;
  serializeJson(jsonDoc, jsonResponse);
  return jsonResponse;
}

void Logger::clearLogs()
{
  currentIndex = 0;
  totalEntries = 0;
  // Clear the buffer
  for (int i = 0; i < MAX_LOG_ENTRIES; i++)
  {
    logBuffer[i].uuid = "";
    logBuffer[i].timestamp = 0;
    logBuffer[i].message = "";
  }
}

int Logger::getLogCount()
{
  return totalEntries;
}