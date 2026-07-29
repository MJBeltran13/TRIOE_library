#include <WiFi.h>
#include <TRIOE.h>
#include <TRIOE_ROOT_CA.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* TELEMETRY_URL = "https://hub.trioe.dev/api/v1/devices/YOUR_DEVICE_ID/telemetry/";
const char* API_KEY = "YOUR_DEVICE_API_KEY";
TrioeClient trioe(TELEMETRY_URL, API_KEY);

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  trioe.setCACert(TRIOE_ROOT_CA);
  trioe.setReportingInterval(30000); // Send at most once every 30 seconds (value is in milliseconds).
  trioe.setChangeThreshold(0.2f);    // Send numeric values only after they change by 0.2 or more.
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

  trioe.loop();  // No delay(): sends/retries only when due.
}
