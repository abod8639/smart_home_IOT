#include "http_handlers.h"
#include "globals.h"
#include "gpio_manager.h"
#include <WiFi.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <Update.h>
#include <IRutils.h>

// Forward declarations of individual route handlers
void handleOptions();
void handlePing();
void handleGetSensors();
void handleDigitalControl();
void handleAnalogControl();
void handleAcControl();
void handleSystemInfo();
void handleOtaUpdate();
void handleOtaStatus();
void handleIrLearn();
void handleIrSend();
void handleNotFound();

void sendJson(int code, JsonDocument& doc) {
  String body;
  serializeJson(doc, body);
  server.send(code, "application/json", body);
}

void sendSimple(int code, const char* status, const char* message) {
  StaticJsonDocument<128> doc;
  doc["status"]  = status;
  doc["message"] = message;
  sendJson(code, doc);
}

void setCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type,Accept");
}

void logRequest() {
  String method = "";
  switch (server.method()) {
    case HTTP_GET:     method = "GET"; break;
    case HTTP_POST:    method = "POST"; break;
    case HTTP_DELETE:  method = "DELETE"; break;
    case HTTP_PUT:     method = "PUT"; break;
    case HTTP_PATCH:   method = "PATCH"; break;
    case HTTP_OPTIONS: method = "OPTIONS"; break;
    default:           method = "UNKNOWN"; break;
  }
  // Serial.printf("[HTTP] %s %s request received\n", method.c_str(), server.uri().c_str());
}

void handleOptions() {
  logRequest();
  setCorsHeaders();
  server.send(204);
}

void handlePing() {
  logRequest();
  setCorsHeaders();
  StaticJsonDocument<128> doc;
  doc["status"]  = "ok";
  doc["device"]  = DEVICE_NAME;
  doc["version"] = FIRMWARE_VERSION;
  doc["uptime"]  = millis() / 1000;
  sendJson(200, doc);
}

void handleGetSensors() {
  logRequest();
  setCorsHeaders();

  float temp     = dht.readTemperature();
  float humidity = dht.readHumidity();

  StaticJsonDocument<1024> doc;

  // Environmental sensors
  if (!isnan(temp)) {
    doc["temperature"] = round(temp * 10.0) / 10.0;
  } else {
    doc["temperature"] = nullptr;
  }

  if (!isnan(humidity)) {
    doc["humidity"] = round(humidity * 10.0) / 10.0;
  } else {
    doc["humidity"] = nullptr;
  }

  // All pin states
  JsonObject pins = doc.createNestedObject("pins");
  for (uint8_t i = 0; i < PIN_COUNT; i++) {
    uint8_t gpio = PIN_MAP[i].gpio;
    if (PIN_MAP[i].isPwm) {
      pins[PIN_MAP[i].label] = analogValue[gpio];
    } else {
      pins[PIN_MAP[i].label] = digitalState[gpio] ? 1 : 0;
    }
  }

  doc["wifi_rssi"] = WiFi.RSSI();
  doc["heap_free"] = esp_get_free_heap_size();
  doc["target_temperature"] = targetTemperature;

  sendJson(200, doc);
}

void handleDigitalControl() {
  logRequest();
  setCorsHeaders();

  if (!server.hasArg("plain")) {
    Serial.println("  -> Error: Missing JSON body");
    sendSimple(400, "error", "Missing JSON body");
    return;
  }

  String body = server.arg("plain");
  Serial.print("  -> Body: ");
  Serial.println(body);

  StaticJsonDocument<128> req;
  DeserializationError err = deserializeJson(req, body);
  if (err) {
    Serial.print("  -> JSON Deserialization error: ");
    Serial.println(err.c_str());
    sendSimple(400, "error", "Invalid JSON");
    return;
  }

  if (!req.containsKey("pin") || !req.containsKey("value")) {
    Serial.println("  -> Error: Missing pin or value parameter");
    sendSimple(400, "error", "Fields 'pin' and 'value' are required");
    return;
  }

  uint8_t gpio  = req["pin"].as<uint8_t>();
  int     value = req["value"].as<int>();

  Serial.printf("  -> Requesting Pin: %d, Value: %d\n", gpio, value);

  const PinConfig* pin = findPin(gpio);
  if (!pin) {
    Serial.printf("  -> Error: GPIO %d not in PIN_MAP\n", gpio);
    sendSimple(404, "error", "GPIO pin not registered in PIN_MAP");
    return;
  }

  if (pin->isPwm) {
    Serial.println("  -> Error: Requested pin is configured as PWM");
    sendSimple(400, "error", "This pin is PWM. Use /control/analog instead");
    return;
  }

  writePin(gpio, value);
  Serial.printf("  -> Pin %d successfully set to %d\n", gpio, value);

  StaticJsonDocument<128> res;
  res["status"]  = "ok";
  res["pin"]     = gpio;
  res["label"]   = pin->label;
  res["value"]   = digitalState[gpio] ? 1 : 0;
  sendJson(200, res);
}

void handleAnalogControl() {
  logRequest();
  setCorsHeaders();

  if (!server.hasArg("plain")) {
    Serial.println("  -> Error: Missing JSON body");
    sendSimple(400, "error", "Missing JSON body");
    return;
  }

  String body = server.arg("plain");
  Serial.print("  -> Body: ");
  Serial.println(body);

  StaticJsonDocument<128> req;
  DeserializationError err = deserializeJson(req, body);
  if (err) {
    Serial.print("  -> JSON Deserialization error: ");
    Serial.println(err.c_str());
    sendSimple(400, "error", "Invalid JSON");
    return;
  }

  if (!req.containsKey("pin") || !req.containsKey("value")) {
    Serial.println("  -> Error: Missing pin or value parameter");
    sendSimple(400, "error", "Fields 'pin' and 'value' are required");
    return;
  }

  uint8_t gpio  = req["pin"].as<uint8_t>();
  int     value = constrain(req["value"].as<int>(), 0, 255);

  Serial.printf("  -> Requesting Pin: %d, PWM Value: %d\n", gpio, value);

  const PinConfig* pin = findPin(gpio);
  if (!pin) {
    Serial.printf("  -> Error: GPIO %d not in PIN_MAP\n", gpio);
    sendSimple(404, "error", "GPIO pin not registered in PIN_MAP");
    return;
  }

  if (!pin->isPwm) {
    Serial.println("  -> Error: Requested pin is configured as digital");
    sendSimple(400, "error", "This pin is digital. Use /control/digital instead");
    return;
  }

  writePin(gpio, value);

  StaticJsonDocument<128> res;
  res["status"] = "ok";
  res["pin"]    = gpio;
  res["label"]  = pin->label;
  res["value"]  = analogValue[gpio];
  sendJson(200, res);
}

void handleAcControl() {
  logRequest();
  setCorsHeaders();

  if (!server.hasArg("plain")) {
    Serial.println("  -> Error: Missing JSON body");
    sendSimple(400, "error", "Missing JSON body");
    return;
  }

  String body = server.arg("plain");
  Serial.print("  -> Body: ");
  Serial.println(body);

  StaticJsonDocument<128> req;
  DeserializationError err = deserializeJson(req, body);
  if (err) {
    Serial.print("  -> JSON Deserialization error: ");
    Serial.println(err.c_str());
    sendSimple(400, "error", "Invalid JSON");
    return;
  }

  if (req.containsKey("target_temp")) {
    targetTemperature = req["target_temp"].as<int>();
    Serial.printf("  -> Target Temperature set to: %d\n", targetTemperature);

    // Save to preferences
    prefs.begin("pins", false);
    prefs.putInt("target_temp", targetTemperature);
    prefs.end();
  }

  if (req.containsKey("isOn")) {
    bool isOn = req["isOn"].as<bool>();
    Serial.printf("  -> AC state set to: %s\n", isOn ? "ON" : "OFF");
    writePin(19, isOn ? 1 : 0);
  }

  StaticJsonDocument<128> res;
  res["status"]             = "ok";
  res["target_temperature"] = targetTemperature;
  res["isOn"]               = digitalState[19];
  sendJson(200, res);
}

void handleSystemInfo() {
  logRequest();
  setCorsHeaders();
  StaticJsonDocument<256> doc;
  doc["firmware"]   = FIRMWARE_VERSION;
  doc["device"]     = DEVICE_NAME;
  doc["chip_model"] = ESP.getChipModel();
  doc["chip_cores"] = ESP.getChipCores();
  doc["cpu_mhz"]    = ESP.getCpuFreqMHz();
  doc["flash_mb"]   = ESP.getFlashChipSize() / (1024 * 1024);
  doc["heap_total"] = ESP.getHeapSize();
  doc["heap_free"]  = esp_get_free_heap_size();
  doc["uptime_s"]   = millis() / 1000;
  doc["ip_address"] = WiFi.localIP().toString();
  doc["mac"]        = WiFi.macAddress();
  sendJson(200, doc);
}

void handleOtaUpdate() {
  logRequest();
  setCorsHeaders();

  if (otaState == OtaState::IN_PROGRESS) {
    sendSimple(409, "error", "OTA already in progress");
    return;
  }

  if (!server.hasArg("plain")) {
    sendSimple(400, "error", "Missing JSON body with 'url' field");
    return;
  }

  StaticJsonDocument<256> req;
  DeserializationError err = deserializeJson(req, server.arg("plain"));
  if (err || !req.containsKey("url")) {
    sendSimple(400, "error", "Invalid JSON — 'url' field required");
    return;
  }

  String firmwareUrl = req["url"].as<String>();
  if (!firmwareUrl.startsWith("http")) {
    sendSimple(400, "error", "URL must start with http:// or https://");
    return;
  }

  // Acknowledge immediately; OTA runs in background task
  sendSimple(202, "accepted", "OTA update started. Poll /ota/status for progress.");

  // Launch OTA in a FreeRTOS task to avoid blocking the web server
  static String urlBuffer;
  urlBuffer = firmwareUrl;

  xTaskCreatePinnedToCore(
    [](void* param) {
      String url = *((String*)param);

      otaState    = OtaState::IN_PROGRESS;
      otaProgress = 0;
      otaError    = "";

      Serial.printf("[OTA] Starting update from: %s\n", url.c_str());

      // Register progress callback
      Update.onProgress([](size_t done, size_t total) {
        if (total > 0) {
          otaProgress = (int)((done * 100) / total);
          Serial.printf("[OTA] Progress: %d%%\n", otaProgress);
        }
      });

      HTTPClient http;
      http.begin(url);
      int httpCode = http.GET();

      if (httpCode != HTTP_CODE_OK) {
        otaState = OtaState::FAILED;
        otaError = "HTTP error: " + String(httpCode);
        Serial.printf("[OTA] HTTP fetch failed: %d\n", httpCode);
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      int contentLength = http.getSize();
      if (contentLength <= 0) {
        otaState = OtaState::FAILED;
        otaError = "Invalid firmware content length";
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      if (!Update.begin(contentLength, U_FLASH)) {
        otaState = OtaState::FAILED;
        otaError = "OTA begin failed: not enough flash space";
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      WiFiClient* stream    = http.getStreamPtr();
      size_t      written   = Update.writeStream(*stream);

      if (written != (size_t)contentLength) {
        otaState = OtaState::FAILED;
        otaError = "Write mismatch: expected " + String(contentLength) +
                   ", wrote " + String(written);
        Update.abort();
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      if (!Update.end()) {
        otaState = OtaState::FAILED;
        otaError = "OTA end failed: " + String(Update.getError());
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      http.end();
      otaProgress = 100;
      otaState    = OtaState::SUCCESS;

      Serial.println("[OTA] Update complete! Rebooting in 2 seconds...");
      delay(2000);
      ESP.restart();

      vTaskDelete(nullptr);
    },
    "ota_task",
    8192,       // 8KB stack — enough for HTTP + flash
    &urlBuffer,
    1,          // priority
    nullptr,
    0           // run on core 0 (loop runs on core 1)
  );
}

void handleOtaStatus() {
  logRequest();
  setCorsHeaders();

  StaticJsonDocument<128> doc;

  switch (otaState) {
    case OtaState::IDLE:
      doc["state"]    = "idle";
      doc["progress"] = 0;
      break;
    case OtaState::IN_PROGRESS:
      doc["state"]    = "in_progress";
      doc["progress"] = otaProgress;
      break;
    case OtaState::SUCCESS:
      doc["state"]    = "success";
      doc["progress"] = 100;
      break;
    case OtaState::FAILED:
      doc["state"]    = "failed";
      doc["progress"] = otaProgress;
      doc["error"]    = otaError;
      break;
  }

  sendJson(200, doc);
}

void handleIrLearn() {
  logRequest();
  setCorsHeaders();

  irrecv.resume(); // Clear any pending buffer
  delay(100);

  decode_results results;
  unsigned long start = millis();
  bool found = false;

  while (millis() - start < 10000) {
    if (irrecv.decode(&results)) {
      found = true;
      break;
    }
    delay(50);
  }

  if (found) {
    // Format the value as hex string
    char hexBuffer[32];
    sprintf(hexBuffer, "0x%llX", results.value);

    StaticJsonDocument<256> doc;
    doc["status"] = "ok";
    doc["protocol"] = typeToString(results.decode_type);
    doc["value"] = hexBuffer;
    doc["bits"] = results.bits;

    sendJson(200, doc);
    irrecv.resume();
  } else {
    sendSimple(408, "error", "Learning timeout - no signal detected");
  }
}

void handleIrSend() {
  logRequest();
  setCorsHeaders();

  if (!server.hasArg("plain")) {
    sendSimple(400, "error", "Missing JSON body");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> req;
  DeserializationError err = deserializeJson(req, body);
  if (err) {
    sendSimple(400, "error", "Invalid JSON");
    return;
  }

  if (!req.containsKey("protocol") || !req.containsKey("value") || !req.containsKey("bits")) {
    sendSimple(400, "error", "protocol, value, and bits fields are required");
    return;
  }

  String protocol = req["protocol"].as<String>();
  String valueStr = req["value"].as<String>();
  uint16_t bits   = req["bits"].as<uint16_t>();

  uint64_t val = strtoull(valueStr.c_str(), nullptr, 16);
  decode_type_t type = strToDecodeType(protocol.c_str());

  irrecv.disableIRIn();
  delay(50);
  irsend.send(type, val, bits);
  delay(50);
  irrecv.enableIRIn();

  StaticJsonDocument<128> res;
  res["status"] = "ok";
  res["message"] = "IR signal transmitted successfully";
  sendJson(200, res);
}

void handleNotFound() {
  logRequest();
  setCorsHeaders();
  sendSimple(404, "error", "Endpoint not found");
}

void setupRoutes() {
  // CORS pre-flight
  server.onNotFound(handleNotFound);
  server.on("/",                HTTP_OPTIONS, handleOptions);
  server.on("/ping",            HTTP_OPTIONS, handleOptions);
  server.on("/sensors",         HTTP_OPTIONS, handleOptions);
  server.on("/control/digital", HTTP_OPTIONS, handleOptions);
  server.on("/control/analog",  HTTP_OPTIONS, handleOptions);
  server.on("/control/ac",      HTTP_OPTIONS, handleOptions);
  server.on("/control/ir/learn", HTTP_OPTIONS, handleOptions);
  server.on("/control/ir/send",  HTTP_OPTIONS, handleOptions);
  server.on("/ota/update",      HTTP_OPTIONS, handleOptions);
  server.on("/ota/status",      HTTP_OPTIONS, handleOptions);
  server.on("/system/info",     HTTP_OPTIONS, handleOptions);

  // Actual routes
  server.on("/ping",            HTTP_GET,  handlePing);
  server.on("/sensors",         HTTP_GET,  handleGetSensors);
  server.on("/control/digital", HTTP_POST, handleDigitalControl);
  server.on("/control/analog",  HTTP_POST, handleAnalogControl);
  server.on("/control/ac",      HTTP_POST, handleAcControl);
  server.on("/control/ir/learn", HTTP_GET,  handleIrLearn);
  server.on("/control/ir/send",  HTTP_POST, handleIrSend);
  server.on("/ota/update",      HTTP_POST, handleOtaUpdate);
  server.on("/ota/status",      HTTP_GET,  handleOtaStatus);
  server.on("/system/info",     HTTP_GET,  handleSystemInfo);
}
