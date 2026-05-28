#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <WebServer.h>
#include <DHT.h>
#include <Preferences.h>
#include <IRrecv.h>
#include <IRsend.h>

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
#define IR_RECV_PIN       14
#define IR_SEND_PIN       12

extern IRrecv irrecv;
extern IRsend irsend;

#endif // GLOBALS_H
