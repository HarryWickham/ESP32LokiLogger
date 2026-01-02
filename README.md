# LokiLogger

A lightweight Arduino library for sending structured logs to Grafana Loki from ESP32 devices.

## Features

- **Structured Logging**: Send logs with levels (DEBUG, INFO, WARNING, ERROR, CRITICAL) to Loki.
- **Buffering & Batching**: Accumulate logs in a buffer and send them in batches to reduce HTTP requests.
- **Retry Logic**: Automatic retries on network failures with configurable attempts and delays.
- **Immediate Flush**: Option to flush logs immediately for critical messages.
- **Colored Serial Output**: Logs are also printed to Serial with ANSI color codes for easy debugging.
- **Authentication**: Support for basic authentication.
- **NTP Time Sync**: Accurate timestamps using NTP.

## Installation

### PlatformIO

Add the following to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/HarryWickham/ESP32LokiLogger.git
```

### Arduino IDE

1. Download the library from [GitHub](https://github.com/HarryWickham/ESP32LokiLogger).
2. Extract it to your Arduino libraries folder.
3. Restart the Arduino IDE.

## Usage

### Basic Setup

```cpp
#include <WiFi.h>
#include <LokiLogger.h>

LokiLogger logger;

void setup() {
    Serial.begin(115200); // For colored log output
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }

    // Initialize logger
    logger.begin("http://your-loki-server:3100/loki/api/v1/push", "username", "api_key", "MyService", "Device1", "pool.ntp.org");
}

void loop() {
    // Log messages (buffered, also printed to Serial with colors)
    logger.log(LogLevel::INFO, "System started");
    logger.log(LogLevel::ERROR, "Sensor failure");

    // Flush buffer manually
    logger.flush();

    delay(60000); // Log every minute
}
```

### Advanced Usage

```cpp
// Immediate flush for critical logs
logger.log(LogLevel::CRITICAL, "Critical error occurred", true);

// Batch logs automatically flush when buffer is full (default 10 entries)

// Check log result
LogResult result = logger.log(LogLevel::WARNING, "Warning message");
if (result != LogResult::SUCCESS) {
    // Handle error
}
```

## API Reference

### Initialization

```cpp
bool begin(const char* lokiUrl, const char* lokiUser, const char* lokiApiKey, const char* serviceName, const char* deviceLabel, const char* ntpServer);
bool begin(const char* lokiUrl, const char* lokiUser, const char* lokiApiKey, const char* serviceName, const char* deviceLabel);
```

- `lokiUrl`: Loki push endpoint URL.
- `lokiUser` & `lokiApiKey`: Credentials for authentication.
- `serviceName` & `deviceLabel`: Labels for logs.
- `ntpServer`: NTP server for time sync (optional in second overload).

### Logging

```cpp
LogResult log(LogLevel level, const char* message, bool immediateFlush = false);
```

- `level`: Log level (DEBUG, INFO, WARNING, ERROR, CRITICAL).
- `message`: Log message string.
- `immediateFlush`: If true, flushes buffer after logging.

Returns `LogResult` enum: BUFFERED (added to buffer), SUCCESS (immediate flush succeeded), NOT_INITIALIZED, WIFI_DISCONNECTED, HTTP_ERROR, INVALID_RESPONSE.

### Flushing

```cpp
LogResult flush();
```

Sends buffered logs to Loki and clears the buffer.

### Log Levels

- `DEBUG`: Detailed debugging info.
- `INFO`: General information.
- `WARNING`: Warning conditions.
- `ERROR`: Error conditions.
- `CRITICAL`: Critical errors.

## Configuration

- **Buffer Size**: Fixed at 10 entries. When full, auto-flushes.
- **Retries**: 3 attempts with 1-second delays on network failures.
- **Time Sync**: Requires NTP for accurate timestamps.
- **Serial Logging**: Logs are automatically printed to Serial with colors (DEBUG: gray, INFO: green, etc.).

## Dependencies

- ArduinoJson (^7.2.0)

## License

MIT License. See [LICENSE](LICENSE) for details.
