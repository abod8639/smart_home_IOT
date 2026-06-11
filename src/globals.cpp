#include "globals.h"

const PinConfig PIN_MAP[] = {
  { 2,  "relay_1",   false },   // e.g. main lamp relay
  { 18, "relay_2",   false },   // e.g. door lock relay
  { 19, "relay_3",   false },   // e.g. AC relay
  { 21, "relay_4",   false },   // spare relay
  { 22, "pwm_lamp",  true  },   // dimmable lamp (PWM)
  { 23, "pwm_rgb_r", true  },   // RGB LED - Red channel
  { 25, "pwm_rgb_g", true  },   // RGB LED - Green channel
  { 26, "pwm_rgb_b", true  },   // RGB LED - Blue channel
};
const uint8_t PIN_COUNT = sizeof(PIN_MAP) / sizeof(PinConfig);

// ─── Runtime State ────────────────────────────────────────────
bool  digitalState[40]  = {};   // gpio -> on/off
int   analogValue[40]   = {};   // gpio -> 0-255 duty cycle

// ─── OTA State ───────────────────────────────────────────────
volatile OtaState otaState      = OtaState::IDLE;
volatile int      otaProgress   = 0;
String            otaError      = "";
int               targetTemperature = 24; // Default target temp

// ─── AC Timer ────────────────────────────────────────────────
bool              acTimerActive     = false;
unsigned long     acTimerDuration   = 0;
unsigned long     acTimerStartMillis = 0;
String            acTimerIrJson     = "";

// ─── Web Server ──────────────────────────────────────────────
WebServer server(80);

// ─── MQTT Client ─────────────────────────────────────────────
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ─── Preferences (NVS) ───────────────────────────────────────
Preferences prefs;

// ─── DHT Sensor Instance ─────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);
float currentTemp = NAN;
float currentHum = NAN;
unsigned long lastDhtReadTime = 0;

// (Global IrReceiver and IrSender are instantiated automatically by including IRremote.hpp)

