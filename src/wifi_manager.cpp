#include "wifi_manager.h"
#include "globals.h"
#include <WiFi.h>

void setupWiFi() {
  Serial.print("[WiFi] Connecting to: ");
  Serial.println(WIFI_SSID);
  Serial.printf("[WiFi] ip = %s\n", WiFi.localIP().toString().c_str());
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(DEVICE_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] Signal RSSI: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println("\n[WiFi] FAILED to connect. Check credentials.");
    // Optional: fall back to AP mode for configuration
    WiFi.softAP(DEVICE_NAME "_AP", "smarthome123");
    Serial.printf("[WiFi] AP Mode: %s\n", WiFi.softAPIP().toString().c_str());
  }
}

void wifiWatchdog() {
  static unsigned long lastWifiCheck = 0;
  const unsigned long WIFI_CHECK_INTERVAL = 15000; // 15s

  if (millis() - lastWifiCheck < WIFI_CHECK_INTERVAL) return;
  lastWifiCheck = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection lost. Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}
