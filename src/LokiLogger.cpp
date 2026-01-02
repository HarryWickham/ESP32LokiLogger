#include <ArduinoJson.h>
#include "LokiLogger.h"

// Constructor
LokiLogger::LokiLogger()
{
  _bufferSize = 1024;
  _initialized = false;
  _lokiUrl = "";
  _lokiUser = "";
  _lokiApiKey = "";
  _serviceName = "";
  _deviceLabel = "";
}

// Destructor
LokiLogger::~LokiLogger()
{
  _httpClient.end();
}

bool LokiLogger::begin(const char *lokiUrl, const char *lokiUser, const char *lokiApiKey, const char *serviceName, const char *deviceLabel, const char *ntpServer)
{
  _lokiUrl = String(lokiUrl);
  _lokiUser = String(lokiUser);
  _lokiApiKey = String(lokiApiKey);
  _serviceName = String(serviceName);
  _deviceLabel = String(deviceLabel);

  configTime(0, 0, ntpServer);

  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  if (tp.tv_sec < 1609459200) // Check if time is before Jan 1, 2021
  {
    return false; // Time not set correctly
  }

  _initialized = true;
  return _initialized;
}

bool LokiLogger::log(LogLevel level, const char *message)
{
  if (!_initialized)
  {
    Serial.println("Logger not initialized");
    return false; // Not initialized
  }

  String logEntryStr = _formatLogEntry(level, message);

  Serial.println("Sending log entry: " + logEntryStr);

  _httpClient.begin(_lokiUrl);
  _httpClient.addHeader("Content-Type", "application/json");
  if (_lokiUser.length() > 0 && _lokiApiKey.length() > 0)
  {
    _httpClient.setAuthorization(_lokiUser.c_str(), _lokiApiKey.c_str());
  }

  int httpResponseCode = _httpClient.POST(logEntryStr);
  _httpClient.end();

  Serial.printf("HTTP Response code: %d\n", httpResponseCode);

  return (httpResponseCode == 204); // Loki returns HTTP 204 No Content on success
}

String LokiLogger::_formatLogEntry(LogLevel level, const char *message)
{
  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);

  // Construct timestamp correctly
  char timestamp[30];
  snprintf(timestamp, sizeof(timestamp), "%lu%09lu", (unsigned long)tp.tv_sec, (unsigned long)tp.tv_nsec);

  // Build JSON
  JsonDocument doc;
  JsonObject streams0 = doc["streams"].add<JsonObject>();
  JsonObject stream = streams0["stream"].to<JsonObject>();

  stream["service"] = _serviceName;
  stream["device"] = _deviceLabel;
  stream["level"] = _stringLevel(level);

  JsonArray values0 = streams0["values"].add<JsonArray>();
  values0.add(timestamp);
  values0.add(message);

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  return jsonPayload;
}

String LokiLogger::_stringLevel(LogLevel level)
{
  switch (level)
  {
  case LogLevel::DEBUG:
    return "DEBUG";
  case LogLevel::INFO:
    return "INFO";
  case LogLevel::WARNING:
    return "WARNING";
  case LogLevel::ERROR:
    return "ERROR";
  case LogLevel::CRITICAL:
    return "CRITICAL";
  default:
    return "UNKNOWN";
  }
}