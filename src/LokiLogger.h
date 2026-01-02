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
   * @return true
   * @return false
   */
  bool log(LogLevel level, const char *message);

  /**
   * @brief Format a log entry as a JSON string for Loki
   *
   * @param level The severity level of the log message
   * @param message The log message content
   * @return String The formatted JSON log entry
   */
  String _formatLogEntry(LogLevel level, const char *message);

private:
  String _lokiUrl;
  String _lokiUser;
  String _lokiApiKey;
  String _serviceName;
  String _deviceLabel;
  HTTPClient _httpClient;
  uint16_t _bufferSize;
  bool _initialized;

  String _stringLevel(LogLevel level);
};

#endif // LOKI_LOGGER_H