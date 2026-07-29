#include <WiFi.h>
#include <TRIOE.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* TELEMETRY_URL = "https://your-hub.example/api/v1/devices/YOUR_DEVICE_ID/telemetry/";
const char* API_KEY = "YOUR_8_DIGIT_API_KEY";
TrioeClient trioe(TELEMETRY_URL, API_KEY);

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  trioe.setReportingInterval(30000); // Send at most once every 30 seconds (value is in milliseconds).
  trioe.setChangeThreshold(0.2f);    // Send numeric values only after they change by 0.2 or more.
  trioe.begin();
}

void loop() {
  float temperature = 20.0f + (analogRead(34) / 4095.0f) * 15.0f;
  trioe.addReading("temperature", temperature, "C");
  trioe.addReading("online", WiFi.status() == WL_CONNECTED);
  trioe.loop();  // No delay(): sends/retries only when due.
}
