#include "http_handlers.h"
#include "globals.h"
#include <WiFi.h>

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
