#ifndef LOKI_LOGGER_H
#define LOKI_LOGGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

enum class LogLevel : uint8_t
{
  DEBUG = 0,   // Detailed information for debugging
  INFO = 1,    // General information
  WARNING = 2, // Warning conditions
  ERROR = 3,   // Error conditions
  CRITICAL = 4 // Critical error conditions
};

enum class LogResult : uint8_t
{
  SUCCESS = 0,
  BUFFERED = 1,
  NOT_INITIALIZED = 2,
  WIFI_DISCONNECTED = 3,
  HTTP_ERROR = 4,
  INVALID_RESPONSE = 5
};

static const uint8_t MAX_BUFFER_SIZE = 10;
static const uint16_t MAX_MESSAGE_LENGTH = 256;
static const uint16_t MAX_TIMESTAMP_LENGTH = 30;

struct LogEntry
{
  LogLevel level;
  char message[MAX_MESSAGE_LENGTH];
  char timestamp[MAX_TIMESTAMP_LENGTH];
};

class LokiLogger
{
public:
  LokiLogger();
  ~LokiLogger();

  /**
   * @brief Initialise connection to Loki server
   *
   * @param lokiUrl The URL of the Loki server
   * @param lokiUser The username for Loki authentication
   * @param lokiApiKey The API key for Loki authentication
   * @param serviceName The name of the service sending logs
   * @param deviceLabel The label identifying the device
   * @param ntpServer The NTP server for time synchronization
   * @return true
   * @return false
   */
  bool begin(const char *lokiUrl, const char *lokiUser, const char *lokiApiKey, const char *serviceName, const char *deviceLabel, const char *ntpServer);

  /**
   * @brief Initialise connection to Loki server without NTP time configuration
   *
   * @param lokiUrl The URL of the Loki server
   * @param lokiUser The username for Loki authentication
   * @param lokiApiKey The API key for Loki authentication
   * @param serviceName The name of the service sending logs
   * @param deviceLabel The label identifying the device
   * @return true
   * @return false
   */
  bool begin(const char *lokiUrl, const char *lokiUser, const char *lokiApiKey, const char *serviceName, const char *deviceLabel);

  /**
   * @brief Log a message with a specified log level
   *
   * @param level The severity level of the log message
   * @param message The log message content
   * @param immediateFlush If true, flush the buffer immediately after adding this log
   * @return LogResult: BUFFERED if added to buffer (or SUCCESS if immediateFlush succeeds), or error codes
   */
  LogResult log(LogLevel level, const char *message, bool immediateFlush = false);

  /**
   * @brief Flush the buffered logs to Loki
   *
   * @return LogResult indicating success or specific error
   */
  LogResult flush();

private:
  String _lokiUrl;
  String _lokiUser;
  String _lokiApiKey;
  String _serviceName;
  String _deviceLabel;
  HTTPClient _httpClient;
  uint16_t _bufferSize;
  bool _initialized;
  uint8_t _maxRetries;
  uint16_t _retryDelayMs;
  LogEntry _buffer[MAX_BUFFER_SIZE];
  uint8_t _bufferIndex;

  const char *_stringLevel(LogLevel level);
  String _formatBatchLogEntry();
  void _getTimestamp(char *buffer, size_t size);
  LogResult _sendHttpRequest(const String &payload);
  const char *_getColorCode(LogLevel level);
};

#endif // LOKI_LOGGER_H