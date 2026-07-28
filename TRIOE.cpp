#include "TRIOE.h"
#include <math.h>

namespace { constexpr uint8_t kNumber = 0, kInteger = 1, kBoolean = 2, kString = 3; }

TrioeClient::TrioeClient(const char* endpoint, const char* apiKey)
    : _endpoint(endpoint ? endpoint : ""), _apiKey(apiKey ? apiKey : "") {}

bool TrioeClient::begin() {
  if (_endpoint.length() == 0 || _apiKey.length() == 0) return false;
  if (_insecureTls) _secureClient.setInsecure();
  _http.setReuse(true); _started = true; _nextPublishAt = _nextCommandPollAt = millis(); return true;
}
void TrioeClient::loop() {
  if (!_started) return;
  _connectionStatus = WiFi.status() == WL_CONNECTED ? ConnectionStatus::Connected : ConnectionStatus::Disconnected;
  if (_connectionStatus != ConnectionStatus::Connected) return;
  const uint32_t now = millis();
  if (_queueCount && isDue(now, _nextPublishAt) && isDue(now, _retryAt)) publishBatch();
  if (_commandHandler && _commandEndpoint.length() && isDue(now, _nextCommandPollAt)) pollCommands();
}
bool TrioeClient::addReading(const char* name, float value, const char* unit) { return queueNumeric(name, value, kNumber, unit); }
bool TrioeClient::addReading(const char* name, int value, const char* unit) { return queueNumeric(name, value, kInteger, unit); }
bool TrioeClient::addReading(const char* name, bool value, const char* unit) { return queueText(name, value ? "true" : "false", kBoolean, unit); }
bool TrioeClient::addReading(const char* name, const char* value, const char* unit) { return queueText(name, value, kString, unit); }
bool TrioeClient::publish() { return publishBatch(); }
bool TrioeClient::publishBatch() {
  if (!_started || !_queueCount) return false;
  if (WiFi.status() != WL_CONNECTED) { _connectionStatus = ConnectionStatus::Disconnected; _deliveryStatus = DeliveryStatus::Queued; return false; }
  return isDue(millis(), _retryAt) ? postBatch() : false;
}
bool TrioeClient::pollCommands() {
  if (!_started || !_commandHandler || !_commandEndpoint.length() || WiFi.status() != WL_CONNECTED) return false;
  _nextCommandPollAt = millis() + _commandPollIntervalMs;
  if (!_http.begin(_secureClient, _commandEndpoint)) return false;
  _http.setReuse(true); _http.addHeader("Authorization", "Bearer " + _apiKey); _lastHttpStatus = _http.GET();
  if (_lastHttpStatus < 200 || _lastHttpStatus >= 300) { _http.end(); return false; }
  StaticJsonDocument<TRIOE_JSON_DOCUMENT_SIZE> doc;
  const DeserializationError error = deserializeJson(doc, _http.getStream()); _http.end();
  if (error) return false;
  for (JsonVariantConst command : doc["commands"].as<JsonArrayConst>()) _commandHandler(command);
  return true;
}
void TrioeClient::setCommandEndpoint(const char* endpoint) { _commandEndpoint = endpoint ? endpoint : ""; }
void TrioeClient::setCommandHandler(CommandHandler handler) { _commandHandler = handler; }
void TrioeClient::setReportingInterval(uint32_t ms) { _reportingIntervalMs = ms; }
void TrioeClient::setCommandPollInterval(uint32_t ms) { _commandPollIntervalMs = ms; }
void TrioeClient::setChangeThreshold(float value) { _changeThreshold = value < 0 ? 0 : value; }
void TrioeClient::setRetryDelays(uint32_t base, uint32_t maximum) { _retryBaseDelayMs = base; _retryMaxDelayMs = maximum < base ? base : maximum; }
void TrioeClient::setInsecureTls(bool enabled) { _insecureTls = enabled; }
void TrioeClient::setCACert(const char* certificate) { if (certificate) { _secureClient.setCACert(certificate); _insecureTls = false; } }
TrioeClient::ConnectionStatus TrioeClient::connectionStatus() const { return _connectionStatus; }
TrioeClient::DeliveryStatus TrioeClient::deliveryStatus() const { return _deliveryStatus; }
bool TrioeClient::isConnected() const { return _connectionStatus == ConnectionStatus::Connected; }
bool TrioeClient::isDeliveryPending() const { return _queueCount > 0; }
uint8_t TrioeClient::queuedCount() const { return _queueCount; }
uint32_t TrioeClient::lastAcceptedSequence() const { return _lastAcceptedSequence; }
int TrioeClient::lastHttpStatus() const { return _lastHttpStatus; }
bool TrioeClient::queueNumeric(const char* name, double value, uint8_t type, const char* unit) {
  if (!name || !isfinite(value) || _pendingSequence != 0) return false; Reading reading{}; copyText(reading.name, sizeof(reading.name), name); copyText(reading.unit, sizeof(reading.unit), unit); reading.number = value; reading.type = type;
  if (!valueChanged(reading)) return false;
  for (uint8_t i = 0; i < _queueCount; ++i) if (!strcmp(_queue[i].name, reading.name)) { _queue[i] = reading; _deliveryStatus = DeliveryStatus::Queued; return true; }
  if (_queueCount >= TRIOE_MAX_QUEUED_READINGS) { _deliveryStatus = DeliveryStatus::Failed; return false; }
  _queue[_queueCount++] = reading; _deliveryStatus = DeliveryStatus::Queued; return true;
}
bool TrioeClient::queueText(const char* name, const char* value, uint8_t type, const char* unit) {
  if (!name || !value || _pendingSequence != 0) return false; Reading reading{}; copyText(reading.name, sizeof(reading.name), name); copyText(reading.unit, sizeof(reading.unit), unit); copyText(reading.text, sizeof(reading.text), value); reading.type = type;
  if (!valueChanged(reading)) return false;
  for (uint8_t i = 0; i < _queueCount; ++i) if (!strcmp(_queue[i].name, reading.name)) { _queue[i] = reading; _deliveryStatus = DeliveryStatus::Queued; return true; }
  if (_queueCount >= TRIOE_MAX_QUEUED_READINGS) { _deliveryStatus = DeliveryStatus::Failed; return false; }
  _queue[_queueCount++] = reading; _deliveryStatus = DeliveryStatus::Queued; return true;
}
bool TrioeClient::valueChanged(const Reading& candidate) const {
  for (uint8_t i = 0; i < _deliveredCount; ++i) { const Reading& old = _delivered[i]; if (strcmp(old.name, candidate.name) || old.type != candidate.type) continue; return candidate.type <= kInteger ? fabs(candidate.number - old.number) >= _changeThreshold : strcmp(candidate.text, old.text) != 0; }
  return true;
}
void TrioeClient::rememberDelivered(const Reading& reading) {
  for (uint8_t i = 0; i < _deliveredCount; ++i) if (!strcmp(_delivered[i].name, reading.name)) { _delivered[i] = reading; return; }
  if (_deliveredCount < TRIOE_MAX_QUEUED_READINGS) _delivered[_deliveredCount++] = reading;
}
bool TrioeClient::postBatch() {
  _deliveryStatus = DeliveryStatus::Sending; if (_pendingSequence == 0) _pendingSequence = ++_sequence; StaticJsonDocument<TRIOE_JSON_DOCUMENT_SIZE> doc; doc["sequence"] = _pendingSequence; JsonArray values = doc.createNestedArray("readings");
  for (uint8_t i = 0; i < _queueCount; ++i) { const Reading& r = _queue[i]; JsonObject item = values.createNestedObject(); item["name"] = r.name; item["unit"] = r.unit; if (r.type == kNumber) { item["type"] = "number"; item["value"] = r.number; } else if (r.type == kInteger) { item["type"] = "integer"; item["value"] = static_cast<int>(r.number); } else if (r.type == kBoolean) { item["type"] = "boolean"; item["value"] = !strcmp(r.text, "true"); } else { item["type"] = "string"; item["value"] = r.text; } }
  if (doc.overflowed()) { _deliveryStatus = DeliveryStatus::Failed; return false; }
  String payload; serializeJson(doc, payload); if (!_http.begin(_secureClient, _endpoint)) { scheduleRetry(); return false; }
  _http.setReuse(true); _http.addHeader("Content-Type", "application/json"); _http.addHeader("Authorization", "Bearer " + _apiKey); _lastHttpStatus = _http.POST(payload); _http.end();
  if (_lastHttpStatus < 200 || _lastHttpStatus >= 300) { scheduleRetry(); return false; }
  for (uint8_t i = 0; i < _queueCount; ++i) rememberDelivered(_queue[i]); _queueCount = 0; _lastAcceptedSequence = _pendingSequence; _pendingSequence = 0; _retryAttempt = 0; _retryAt = 0; _nextPublishAt = millis() + _reportingIntervalMs; _deliveryStatus = DeliveryStatus::Delivered; return true;
}
void TrioeClient::scheduleRetry() { const uint8_t exponent = _retryAttempt > 6 ? 6 : _retryAttempt++; uint32_t delayMs = _retryBaseDelayMs << exponent; if (delayMs > _retryMaxDelayMs || delayMs < _retryBaseDelayMs) delayMs = _retryMaxDelayMs; _retryAt = millis() + delayMs + (delayMs / 4 ? random(delayMs / 4) : 0); _deliveryStatus = DeliveryStatus::Retrying; }
bool TrioeClient::isDue(uint32_t now, uint32_t dueAt) const { return static_cast<int32_t>(now - dueAt) >= 0; }
void TrioeClient::copyText(char* target, size_t size, const char* source) { if (!source) { target[0] = '\0'; return; } strlcpy(target, source, size); }
