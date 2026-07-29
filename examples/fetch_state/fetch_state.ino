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

  trioe.setStatePollInterval(5000);
  if (!trioe.begin()) {
    Serial.println("TRIOE setup failed: check endpoint, API key, and CA certificate.");
  }
}

void loop() {
  trioe.loop();
}
