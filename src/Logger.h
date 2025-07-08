#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <UUID.h>

struct LogEntry
{
  String uuid;
  unsigned long timestamp;
  String message;
};

class Logger
{
private:
  static const int MAX_LOG_ENTRIES = 50;
  LogEntry logBuffer[MAX_LOG_ENTRIES];
  int currentIndex;
  int totalEntries;
  UUID uuidGenerator;

  Logger();

  // Delete copy constructor and assignment operator
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

public:
  // Singleton access method
  static Logger &getInstance();

  void log(const String &message);
  void log(const char *message);
  void logf(const char *format, ...);
  String getLogsAsJson();
  void clearLogs();
  int getLogCount();
};

// Convenience macro for easier access
#define logger Logger::getInstance()

#endif // LOGGER_H