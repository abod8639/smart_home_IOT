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

// PWM ledcChannel offset — channels 0..7 reserved for PWM pins
uint8_t pwmChannelIndex = 0;
uint8_t pinToPwmChannel[256] = {};  // gpio -> ledc channel lookup

// ─── Runtime State ────────────────────────────────────────────
bool  digitalState[256]  = {};   // gpio -> on/off
int   analogValue[256]   = {};   // gpio -> 0-255 duty cycle

// ─── OTA State ───────────────────────────────────────────────
volatile OtaState otaState      = OtaState::IDLE;
volatile int      otaProgress   = 0;
String            otaError      = "";
int               targetTemperature = 24; // Default target temp

// ─── Web Server ──────────────────────────────────────────────
WebServer server(80);

// ─── Preferences (NVS) ───────────────────────────────────────
Preferences prefs;

// ─── DHT Sensor Instance ─────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);

// ─── IR Receiver and Transmitter Instances ───────────────────
IRrecv irrecv(IR_RECV_PIN);
IRsend irsend(IR_SEND_PIN);
