#include <Arduino.h>
// Must include IRremote.hpp here (without USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE)
// so this TU instantiates IrReceiver / IrSender for the whole project.
#include <IRremote.hpp>
#include "globals.h"  // IR pin macros defined here (IR_RECEIVE_PIN, IR_SEND_PIN)
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

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND_PIN);
  Serial.printf("[IR] Receiver on GPIO%d, Transmitter on GPIO%d\n", IR_RECEIVE_PIN, IR_SEND_PIN);

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