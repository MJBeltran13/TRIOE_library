#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#ifndef TRIOE_MAX_QUEUED_READINGS
#define TRIOE_MAX_QUEUED_READINGS 16
#endif

#ifndef TRIOE_JSON_DOCUMENT_SIZE
#define TRIOE_JSON_DOCUMENT_SIZE 4096
#endif

class TrioeClient {
 public:
  enum class ConnectionStatus : uint8_t { Disconnected, Connecting, Connected };
  enum class DeliveryStatus : uint8_t { Idle, Queued, Sending, Delivered, Retrying, Failed };
  using CommandHandler = void (*)(JsonVariantConst command);

  TrioeClient(const char* endpoint, const char* apiKey);
  bool begin();
  void loop();

  bool addReading(const char* name, float value, const char* unit = nullptr);
  bool addReading(const char* name, int value, const char* unit = nullptr);
  bool addReading(const char* name, bool value, const char* unit = nullptr);
  bool addReading(const char* name, const char* value, const char* unit = nullptr);
  bool publish();
  bool publishBatch();
  bool pollCommands();

  void setCommandEndpoint(const char* endpoint);
  void setCommandHandler(CommandHandler handler);
  void setReportingInterval(uint32_t intervalMs);
  void setCommandPollInterval(uint32_t intervalMs);
  void setChangeThreshold(float threshold);
  void setRetryDelays(uint32_t baseDelayMs, uint32_t maxDelayMs);
  void setCACert(const char* certificate);

  ConnectionStatus connectionStatus() const;
  DeliveryStatus deliveryStatus() const;
  bool isConnected() const;
  bool isDeliveryPending() const;
  uint8_t queuedCount() const;
  uint32_t lastAcceptedSequence() const;
  int lastHttpStatus() const;

 private:
  static constexpr size_t kNameLength = 65;
  static constexpr size_t kUnitLength = 51;
  static constexpr size_t kTextLength = 129;
  struct Reading {
    char name[kNameLength];
    char unit[kUnitLength];
    char text[kTextLength];
    double number;
    uint8_t type;
  };

  bool queueNumeric(const char* name, double value, uint8_t type, const char* unit);
  bool queueText(const char* name, const char* value, uint8_t type, const char* unit);
  bool valueChanged(const Reading& candidate) const;
  void rememberDelivered(const Reading& reading);
  bool postBatch();
  void scheduleRetry();
  bool isDue(uint32_t now, uint32_t dueAt) const;
  void copyText(char* destination, size_t destinationSize, const char* source);

  String _endpoint, _commandEndpoint, _apiKey;
  WiFiClientSecure _secureClient;
  HTTPClient _http;
  Reading _queue[TRIOE_MAX_QUEUED_READINGS];
  Reading _delivered[TRIOE_MAX_QUEUED_READINGS];
  uint8_t _queueCount = 0, _deliveredCount = 0, _retryAttempt = 0;
  uint32_t _sequence = 0, _pendingSequence = 0, _lastAcceptedSequence = 0;
  uint32_t _reportingIntervalMs = 30000, _commandPollIntervalMs = 60000;
  uint32_t _nextPublishAt = 0, _nextCommandPollAt = 0, _retryAt = 0;
  uint32_t _retryBaseDelayMs = 1000, _retryMaxDelayMs = 60000;
  float _changeThreshold = 0.0f;
  int _lastHttpStatus = 0;
  bool _started = false, _certificateConfigured = false;
  ConnectionStatus _connectionStatus = ConnectionStatus::Disconnected;
  DeliveryStatus _deliveryStatus = DeliveryStatus::Idle;
  CommandHandler _commandHandler = nullptr;
};
