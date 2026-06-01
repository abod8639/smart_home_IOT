#include <Arduino.h>
// Instantiate IRremote here
#define RAW_BUFFER_LENGTH 750
#include <IRremote.hpp>

#include "globals.h"
#include "wifi_manager.h"
#include "gpio_manager.h"
#include "http_handlers.h"
#include "ota_manager.h"
#include <Matter.h>

// ─── Matter Endpoints ─────────────────────────────────────────
MatterOnOffLight Light1;     // Endpoint 1: Relay 1 (Main Lamp) - GPIO 2
MatterOnOffLight Light2;     // Endpoint 2: Relay 2 (Door Lock) - GPIO 18
MatterOnOffLight Light3;     // Endpoint 3: Relay 3 (AC) - GPIO 19
MatterOnOffLight Light4;     // Endpoint 4: Relay 4 (Spare) - GPIO 21
MatterDimmableLight Light5;  // Endpoint 5: PWM Dimmable Lamp - GPIO 22
MatterColorLight Light6;     // Endpoint 6: RGB LED - GPIO 23, 25, 26

// BOOT Button for decommissioning/factory reset
const uint8_t buttonPin = 0;
uint32_t button_time_stamp = 0;
bool button_state = false;
const uint32_t decommissioningTimeout = 5000;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n========================================");
  Serial.printf("  %s  v%s (Matter Mode)\n", DEVICE_NAME, FIRMWARE_VERSION);
  Serial.println("========================================\n");

  setupGPIO();
  
  // Note: We let Matter handle Wi-Fi connection and stack commissioning.
  // setupWiFi() is commented out to avoid interfering with Matter BLE commissioning.
  // setupWiFi();

  dht.begin();
  Serial.println("[DHT] Sensor initialized");

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  IrSender.begin(); // No arguments needed since IR_SEND_PIN is defined globally
  Serial.printf("[IR] Receiver on GPIO%d, Transmitter on GPIO%d\n", IR_RECEIVE_PIN, IR_SEND_PIN);

  // Restore last-known pin states from NVS flash
  restorePinStates();
  Serial.println("[NVS] Pin states restored\n");


  // Initialize Matter Endpoints
  Light1.begin(false);
  Light2.begin(false);
  Light3.begin(false);
  Light4.begin(false);
  Light5.begin(false, 255);
  espHsvColor_t initialColor = {0, 0, 255};
  Light6.begin(false, initialColor);

  // Setup Matter callbacks
  Light1.onChange([](bool state) {
      Serial.printf("[Matter] Light1 (Relay 1) state: %s\n", state ? "ON" : "OFF");
      writePin(2, state ? 1 : 0);
      return true;
  });
  Light2.onChange([](bool state) {
      Serial.printf("[Matter] Light2 (Relay 2) state: %s\n", state ? "ON" : "OFF");
      writePin(18, state ? 1 : 0);
      return true;
  });
  Light3.onChange([](bool state) {
      Serial.printf("[Matter] Light3 (Relay 3) state: %s\n", state ? "ON" : "OFF");
      writePin(19, state ? 1 : 0);
      return true;
  });
  Light4.onChange([](bool state) {
      Serial.printf("[Matter] Light4 (Relay 4) state: %s\n", state ? "ON" : "OFF");
      writePin(21, state ? 1 : 0);
      return true;
  });
  Light5.onChange([](bool newState, uint8_t newBrightness) {
      Serial.printf("[Matter] Light5 (PWM Lamp) state: %d, brightness: %d\n", newState, newBrightness);
      writePin(22, newState ? newBrightness : 0);
      return true;
  });
  Light6.onChange([](bool newState, espHsvColor_t newColor) {
      Serial.printf("[Matter] Light6 (RGB LED) state: %d, H: %d, S: %d, V: %d\n", 
                    newState, newColor.h, newColor.s, newColor.v);
      if (newState) {
          espRgbColor_t rgb = espHsvColorToRgbColor(newColor);
          writePin(23, rgb.r);
          writePin(25, rgb.g);
          writePin(26, rgb.b);
      } else {
          writePin(23, 0);
          writePin(25, 0);
          writePin(26, 0);
      }
      return true;
  });

  // Start Matter Stack (must be last)
  Matter.begin();
  Serial.println("[Matter] Stack initialized and started");

  // Web Server and OTA must be started AFTER Matter.begin() initializes the TCP/IP (LwIP) stack.
  setupRoutes();
  server.begin();
  Serial.println("[HTTP] Web server started on port 80");

  setupOTA();

  pinMode(buttonPin, INPUT_PULLUP);

  Serial.println("─── Ready ───────────────────────────────");
  Serial.println("  Matter device is advertising over BLE.");
  Serial.println("  SSID: SmartHome-ESP32, default passcode: 20202021");
  Serial.println("─────────────────────────────────────────\n");
}

void loop() {
  server.handleClient();
  handleOTA();
  
  // Handle factory reset / decommissioning via BOOT button
  if (digitalRead(buttonPin) == LOW) {
    if (!button_state) {
      button_state = true;
      button_time_stamp = millis();
    } else if (millis() - button_time_stamp > decommissioningTimeout) {
      Serial.println("[Matter] Decommissioning device...");
      Matter.decommission();
      while (digitalRead(buttonPin) == LOW) {
        delay(10);
      }
      ESP.restart();
    }
  } else {
    button_state = false;
  }
  
  delay(2);  // yield to RTOS scheduler
}