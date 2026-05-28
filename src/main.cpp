#include <Arduino.h>
#include "globals.h"
#include "wifi_manager.h"
#include "gpio_manager.h"
#include "http_handlers.h"
#include "ota_manager.h"

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n========================================");
  Serial.printf("  %s  v%s\n", DEVICE_NAME, FIRMWARE_VERSION);
  Serial.println("========================================\n");

  setupGPIO();
  setupWiFi();

  dht.begin();
  Serial.println("[DHT] Sensor initialized");

  setupRoutes();
  server.begin();
  Serial.println("[HTTP] Web server started on port 80");

  irsend.begin();
  irrecv.enableIRIn();
  Serial.println("[IR] Receiver and transmitter initialized");

  // Restore last-known pin states from NVS flash
  restorePinStates();
  Serial.println("[NVS] Pin states restored\n");

  // Start the standard ArduinoOTA service
  setupOTA();

  Serial.println("─── Ready ───────────────────────────────");
  Serial.printf("  Open in app: http://%s\n", WiFi.localIP().toString().c_str());
  Serial.println("─────────────────────────────────────────\n");
}

void loop() {
  server.handleClient();
  wifiWatchdog();
  handleOTA();
  delay(2);  // yield to RTOS scheduler
}