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
  _maxRetries = 3;
  _retryDelayMs = 1000;
  _bufferIndex = 0;
}

// Destructor
LokiLogger::~LokiLogger()
{
  _httpClient.end();
}

bool LokiLogger::begin(const char *lokiUrl, const char *lokiUser, const char *lokiApiKey, const char *serviceName, const char *deviceLabel, const char *ntpServer)
{
  String url = String(lokiUrl);
  if (!url.startsWith("http://") && !url.startsWith("https://"))
  {
    return false;
  }

  _lokiUrl = url;
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

bool LokiLogger::begin(const char *lokiUrl, const char *lokiUser, const char *lokiApiKey, const char *serviceName, const char *deviceLabel)
{
  String url = String(lokiUrl);
  if (!url.startsWith("http://") && !url.startsWith("https://"))
  {
    return false;
  }

  _lokiUrl = url;
  _lokiUser = String(lokiUser);
  _lokiApiKey = String(lokiApiKey);
  _serviceName = String(serviceName);
  _deviceLabel = String(deviceLabel);

  _initialized = true;
  return _initialized;
}

LogResult LokiLogger::log(LogLevel level, const char *message, bool immediateFlush)
{

  // Print to Serial with color
  Serial.printf("\033[%sm[%s] %s\033[0m\n", _getColorCode(level), _stringLevel(level), message);

  if (!_initialized)
  {
    return LogResult::NOT_INITIALIZED;
  }

  if (_bufferIndex >= MAX_BUFFER_SIZE)
  {
    LogResult flushResult = flush();
    if (flushResult != LogResult::SUCCESS)
    {
      return flushResult;
    }
  }

  _buffer[_bufferIndex].level = level;
  strncpy(_buffer[_bufferIndex].message, message, MAX_MESSAGE_LENGTH - 1);
  _buffer[_bufferIndex].message[MAX_MESSAGE_LENGTH - 1] = '\0';
  _getTimestamp(_buffer[_bufferIndex].timestamp, MAX_TIMESTAMP_LENGTH);
  _bufferIndex++;

  if (immediateFlush)
  {
    return flush();
  }

  return LogResult::BUFFERED;
}

String LokiLogger::_formatBatchLogEntry()
{
  // Build JSON for batch with separate streams for each log level
  JsonDocument doc;
  JsonArray streams = doc["streams"].to<JsonArray>();

  // Group entries by level
  for (uint8_t lvl = 0; lvl <= static_cast<uint8_t>(LogLevel::CRITICAL); ++lvl)
  {
    LogLevel level = static_cast<LogLevel>(lvl);
    bool hasEntries = false;
    for (uint8_t i = 0; i < _bufferIndex; ++i)
    {
      if (_buffer[i].level == level)
      {
        hasEntries = true;
        break;
      }
    }
    if (!hasEntries)
      continue;

    JsonObject streamObj = streams.add<JsonObject>();
    JsonObject streamLabels = streamObj["stream"].to<JsonObject>();
    streamLabels["service"] = _serviceName;
    streamLabels["device"] = _deviceLabel;
    streamLabels["level"] = _stringLevel(level);

    JsonArray values = streamObj["values"].to<JsonArray>();
    for (uint8_t i = 0; i < _bufferIndex; ++i)
    {
      if (_buffer[i].level == level)
      {
        JsonArray value = values.add<JsonArray>();
        value.add(_buffer[i].timestamp);
        value.add(_buffer[i].message);
      }
    }
  }

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  return jsonPayload;
}

void LokiLogger::_getTimestamp(char *buffer, size_t size)
{
  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);

  snprintf(buffer, size, "%lu%09lu", (unsigned long)tp.tv_sec, (unsigned long)tp.tv_nsec);
}

LogResult LokiLogger::flush()
{
  if (!_initialized)
  {
    return LogResult::NOT_INITIALIZED;
  }

  if (_bufferIndex == 0)
  {
    return LogResult::SUCCESS;
  }

  String logEntryStr = _formatBatchLogEntry();

  LogResult sendResult = _sendHttpRequest(logEntryStr);

  if (sendResult == LogResult::SUCCESS)
  {
    _bufferIndex = 0; // Clear buffer on success
  }

  return sendResult;
}

LogResult LokiLogger::_sendHttpRequest(const String &payload)
{
  for (uint8_t attempt = 0; attempt < _maxRetries; ++attempt)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      if (attempt < _maxRetries - 1)
      {
        delay(_retryDelayMs);
        continue;
      }
      return LogResult::WIFI_DISCONNECTED;
    }

    _httpClient.begin(_lokiUrl);
    _httpClient.addHeader("Content-Type", "application/json");
    if (_lokiUser.length() > 0 && _lokiApiKey.length() > 0)
    {
      _httpClient.setAuthorization(_lokiUser.c_str(), _lokiApiKey.c_str());
    }

    int httpResponseCode = _httpClient.POST(payload);
    _httpClient.end();

    if (httpResponseCode == 204)
    {
      return LogResult::SUCCESS;
    }
    else if (httpResponseCode >= 400 && httpResponseCode < 600)
    {
      // Client or server error, don't retry
      return LogResult::HTTP_ERROR;
    }
    else if (httpResponseCode == -1 || httpResponseCode == 0)
    {
      // Network error, retry
      if (attempt < _maxRetries - 1)
      {
        delay(_retryDelayMs);
        continue;
      }
      return LogResult::HTTP_ERROR;
    }
    else
    {
      // Unexpected response, retry
      if (attempt < _maxRetries - 1)
      {
        delay(_retryDelayMs);
        continue;
      }
      return LogResult::INVALID_RESPONSE;
    }
  }

  return LogResult::HTTP_ERROR;
}

const char *LokiLogger::_getColorCode(LogLevel level)
{
  static const char *colorCodes[] = {
      "90", // DEBUG: Bright black
      "32", // INFO: Green
      "33", // WARNING: Yellow
      "31", // ERROR: Red
      "91"  // CRITICAL: Bright red
  };
  return colorCodes[static_cast<uint8_t>(level)];
}

const char *LokiLogger::_stringLevel(LogLevel level)
{
  static const char *levelStrings[] = {
      "DEBUG",
      "INFO",
      "WARNING",
      "ERROR",
      "CRITICAL"};
  return levelStrings[static_cast<uint8_t>(level)];
}