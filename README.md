# TRIOE Arduino Library

`TrioeClient` sends ESP32 telemetry to the TRIOE Hub Phase 4 API. It batches readings, drops unchanged values, retries failed deliveries with jitter, and retains a bounded in-memory queue while Wi-Fi is unavailable.

## Install

Install **ArduinoJson 6** from Library Manager, then add this ESP32 library.

## Quick start

```cpp
#include <WiFi.h>
#include <TRIOE.h>

TrioeClient trioe(
  "https://your-hub.example/api/v1/devices/YOUR_DEVICE_ID/telemetry/",
  "YOUR_8_DIGIT_API_KEY"
);

void setup() {
  WiFi.begin("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");
  trioe.setReportingInterval(30000);
  trioe.setChangeThreshold(0.2f);
  trioe.begin();
}

void loop() {
  trioe.addReading("temperature", 24.6f, "C");
  trioe.loop();
}
```

`publish()` and `publishBatch()` attempt an immediate delivery. `loop()` never calls `delay()`; it sends only when a reporting or retry deadline is due. HTTPS requests themselves are synchronous on ESP32, so keep batches small.

## API

- `begin()` configures the client without waiting for Wi-Fi.
- `addReading()` accepts `float`, `int`, `bool`, or `const char*` values.
- `publish()` and `publishBatch()` send all queued readings as one request.
- `pollCommands()` calls an optional command endpoint and passes command objects to a callback.
- `connectionStatus()`, `deliveryStatus()`, `queuedCount()`, and `lastHttpStatus()` expose runtime state.

Use `setReportingInterval()`, `setChangeThreshold()`, and `setRetryDelays()` to tune delivery. Failed batches stay queued and retry with exponential backoff plus random jitter. Once delivery starts, the queue is locked until the Hub acknowledges it; that keeps retries byte-for-byte consistent with their sequence number.

## Limits and memory

The default queue holds `TRIOE_MAX_QUEUED_READINGS` (16) unique stream names. Each name is 64 bytes, unit 50 bytes, and string value 128 bytes. The JSON document defaults to `TRIOE_JSON_DOCUMENT_SIZE` (4096 bytes); the client rejects an oversized document instead of sending malformed JSON. The Hub accepts up to 250 readings and 256 KB per batch; 16 readings is the recommended ESP32 default.

Override the two macros before including `TRIOE.h` only after checking free heap. `setInsecureTls(true)` is enabled by default for classroom/self-signed deployments; use `setCACert()` in production.
