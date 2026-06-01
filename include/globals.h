#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <WebServer.h>
#include <DHT.h>
#include <Preferences.h>

#ifndef IR_RECEIVE_PIN
#define IR_RECEIVE_PIN 32
#endif

#ifndef IR_SEND_PIN
#define IR_SEND_PIN 33
#endif

#define RAW_BUFFER_LENGTH 750
#define RECORD_GAP_MICROS 12000

// Only include as plain if not already included in the main translation unit
#ifndef IR_REMOTE_HPP
#define IR_MAX_RAW_DATA_BITS 256 // Support up to 256 bits for AC remotes
#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>
#endif

// ─── Firmware Version ────────────────────────────────────────
#define FIRMWARE_VERSION  "1.0.0"
#define DEVICE_NAME       "SmartHome-ESP32"

// ─── WiFi Credentials ────────────────────────────────────────
// Change these to match your network
#define WIFI_SSID         ">_"
#define WIFI_PASSWORD     "Qwertyuio0qwertyuio0"

// ─── DHT Sensor ──────────────────────────────────────────────
#define DHT_PIN           4
#define DHT_TYPE          DHT22

    // ─── GPIO Pin Map Struct ──────────────────────────────────────
    struct PinConfig {
  uint8_t gpio;
  const char* label;
  bool isPwm;       // true = PWM/analog output, false = digital relay
};

extern const PinConfig PIN_MAP[];
extern const uint8_t PIN_COUNT;

// ─── PWM Channel Config ───────────────────────────────────────
#define PWM_FREQ          5000
#define PWM_RESOLUTION    8      // 8-bit: 0-255

extern uint8_t pwmChannelIndex;
extern uint8_t pinToPwmChannel[256];

// ─── Runtime State ────────────────────────────────────────────
extern bool digitalState[256];
extern int analogValue[256];

// ─── OTA State ───────────────────────────────────────────────
enum class OtaState { IDLE, IN_PROGRESS, SUCCESS, FAILED };
extern volatile OtaState otaState;
extern volatile int otaProgress;
extern String otaError;
extern int targetTemperature;

// ─── Web Server ──────────────────────────────────────────────
extern WebServer server;

// ─── Preferences (NVS) ───────────────────────────────────────
extern Preferences prefs;

// ─── DHT Sensor Instance ─────────────────────────────────────
extern DHT dht;

// ─── IR Receiver and Transmitter ─────────────────────────────
// (Pins are configured via macros and handled by the global IrReceiver/IrSender
// instances)

#endif // GLOBALS_H
