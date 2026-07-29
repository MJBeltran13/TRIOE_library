# TRIOE Arduino Library

`TrioeClient` connects a TRIOE device to TRIOE Hub. It publishes sensor readings in batches, fetches current stream values, avoids sending unchanged data, queues readings while offline, and retries failed deliveries automatically.

## Installation

1. Install **ArduinoJson 6** using Arduino Library Manager.
2. Add the TRIOE library to your Arduino libraries folder.
3. Include `WiFi.h` and `TRIOE.h` in your sketch.
4. Create a device in TRIOE Hub and copy its API key. The key is displayed only once.

Never publish real Wi-Fi credentials or API keys in source control.

## Publish data

This example uses dummy data, so it can be tested before physical sensors are connected.

```cpp
#include <WiFi.h>
#include <TRIOE.h>
#include <TRIOE_ROOT_CA.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* TELEMETRY_URL =
    "https://hub.trioe.dev/api/v1/devices/YOUR_DEVICE_ID/telemetry/";
const char* API_KEY = "YOUR_DEVICE_API_KEY";

TrioeClient trioe(TELEMETRY_URL, API_KEY);

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  trioe.setCACert(TRIOE_ROOT_CA);
  trioe.setReportingInterval(30000); // Publish every 30 seconds.
  trioe.setChangeThreshold(0.2f);    // Ignore smaller numeric changes.
  if (!trioe.begin()) {
    Serial.println("TRIOE setup failed: check endpoint, API key, and CA certificate.");
  }
}

void loop() {
  // Dummy data for testing without physical sensors.
  float temperature = 25.5f;
  float humidity = 60.0f;
  const char* message = "Hello from TRIOE";
  bool pumpState = true;

  trioe.addReading("temperature", temperature, "C");
  trioe.addReading("humidity", humidity, "%");
  trioe.addReading("message", message);
  trioe.addReading("pump", pumpState);

  trioe.loop(); // Runs publishing, queueing, and retries when due.
}
```

## Fetch current values

Use named callbacks to receive only the streams your device needs.

```cpp
#include <WiFi.h>
#include <TRIOE.h>
#include <TRIOE_ROOT_CA.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* STATE_URL =
    "https://hub.trioe.dev/api/v1/devices/YOUR_DEVICE_ID/state/";
const char* API_KEY = "YOUR_DEVICE_API_KEY";

TrioeClient trioe(STATE_URL, API_KEY);

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  trioe.setCACert(TRIOE_ROOT_CA);
  trioe.setStateEndpoint(STATE_URL);
  trioe.onReading("temperature", [](float value1) {
    Serial.print("Temperature: ");
    Serial.print(value1);
    Serial.println(" C");
  });

  trioe.onReading("humidity", [](float value2) {
    Serial.print("Humidity: ");
    Serial.print(value2);
    Serial.println(" %");
  });

  trioe.onText("message", [](const char* value3) {
    Serial.print("Message: ");
    Serial.println(value3);
  });

  trioe.onBoolean("pump", [](bool value4) {
    Serial.print("Pump: ");
    Serial.println(value4 ? "ON" : "OFF");
  });

  trioe.setStatePollInterval(5000); // Fetch every five seconds.
  if (!trioe.begin()) {
    Serial.println("TRIOE setup failed: check endpoint, API key, and CA certificate.");
  }
}

void loop() {
  trioe.loop();
}
```

## Supported values

| Data type | Publish | Fetch |
|---|---|---|
| Decimal number | `addReading("temperature", 25.5f, "C")` | `onReading("temperature", callback)` |
| Integer | `addReading("count", 10)` | `onReading("count", callback)` |
| Text | `addReading("message", "Hello from TRIOE")` | `onText("message", callback)` |
| Boolean | `addReading("pump", true)` | `onBoolean("pump", callback)` |

## Main API

- `begin()` initializes the client.
- `loop()` runs scheduled publishing, fetching, command polling, and retries without calling `delay()`.
- `addReading()` queues a `float`, `int`, `bool`, or `const char*` value.
- `publish()` and `publishBatch()` immediately attempt to send queued readings.
- `onReading()` receives numeric current values.
- `onText()` receives text current values.
- `onBoolean()` receives boolean current values.
- `fetchState()` immediately requests current stream values.
- `pollCommands()` requests pending commands.
- `connectionStatus()`, `deliveryStatus()`, `queuedCount()`, and `lastHttpStatus()` expose runtime status.

## Configuration

- `setReportingInterval(milliseconds)` controls scheduled publishing.
- `setChangeThreshold(value)` prevents small numeric changes from being sent.
- `setStatePollInterval(milliseconds)` controls current-state fetching.
- `setRetryDelays(base, maximum)` controls exponential retry timing.
- `setRequestTimeout(milliseconds)` controls the HTTPS request timeout.
- `setCACert(certificate)` is required for secure HTTPS. `TRIOE_ROOT_CA.h` provides the current Hub trust anchor.

## Limits and memory

The default offline queue contains 16 unique stream names. Stream names can use up to 64 characters, units up to 50 characters, and text values up to 128 characters. The default JSON document size is 4096 bytes.

The Hub accepts up to 250 readings and a 256 KB payload per batch, but the default 16-reading batch is recommended for constrained TRIOE devices. Override `TRIOE_MAX_QUEUED_READINGS`, `TRIOE_MAX_READING_HANDLERS`, or `TRIOE_JSON_DOCUMENT_SIZE` only after checking available memory.

The library reuses HTTPS connections when possible, never prints API keys to Serial, and does not use insecure TLS mode.
