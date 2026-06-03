#include "gpio_manager.h"

const PinConfig* findPin(uint8_t gpio) {
  for (uint8_t i = 0; i < PIN_COUNT; i++) {
    if (PIN_MAP[i].gpio == gpio) return &PIN_MAP[i];
  }
  return nullptr;
}

void writePinHardware(uint8_t gpio, int value) {
  const PinConfig* pin = findPin(gpio);
  if (!pin) return;

  if (pin->isPwm) {
    int     duty    = constrain(value, 0, 255);
    ledcWrite(gpio, duty);
    analogValue[gpio] = duty;
    digitalState[gpio] = (duty > 0);
  } else {
    bool on = (value != 0);
    digitalWrite(gpio, on ? HIGH : LOW);
    digitalState[gpio] = on;
  }
}

void writePin(uint8_t gpio, int value) {
  writePinHardware(gpio, value);

  // Persist state across reboots
  prefs.begin("pins", false);
  char key[8];
  snprintf(key, sizeof(key), "p%d", gpio);
  prefs.putInt(key, value);
  prefs.end();
}

void restorePinStates() {
  prefs.begin("pins", true);
  for (uint8_t i = 0; i < PIN_COUNT; i++) {
    uint8_t gpio = PIN_MAP[i].gpio;
    char key[8];
    snprintf(key, sizeof(key), "p%d", gpio);
    int val = prefs.getInt(key, 0);
    writePinHardware(gpio, val);
  }
  targetTemperature = prefs.getInt("target_temp", 24);
  prefs.end();
}

void setupGPIO() {
  for (uint8_t i = 0; i < PIN_COUNT; i++) {
    uint8_t gpio = PIN_MAP[i].gpio;
    if (PIN_MAP[i].isPwm) {
      // In Arduino Core 3.0, we call ledcAttach(pin, freq, resolution)
      // Channels are managed automatically under the hood.
      ledcAttach(gpio, PWM_FREQ, PWM_RESOLUTION);
      Serial.printf("[GPIO] PWM  pin %d (%s) attached\n",
                    gpio, PIN_MAP[i].label);
    } else {
      pinMode(gpio, OUTPUT);
      digitalWrite(gpio, LOW);
      Serial.printf("[GPIO] DOUT pin %d (%s)\n", gpio, PIN_MAP[i].label);
    }
  }
}
