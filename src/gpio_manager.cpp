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
    uint8_t ch      = pinToPwmChannel[gpio];
    int     duty    = constrain(value, 0, 255);
    ledcWrite(ch, duty);
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
  String key = "p" + String(gpio);
  prefs.putInt(key.c_str(), value);
  prefs.end();
}

void restorePinStates() {
  prefs.begin("pins", true);
  for (uint8_t i = 0; i < PIN_COUNT; i++) {
    uint8_t gpio = PIN_MAP[i].gpio;
    String  key  = "p" + String(gpio);
    int     val  = prefs.getInt(key.c_str(), 0);
    writePinHardware(gpio, val);
  }
  targetTemperature = prefs.getInt("target_temp", 24);
  prefs.end();
}

void setupGPIO() {
  for (uint8_t i = 0; i < PIN_COUNT; i++) {
    uint8_t gpio = PIN_MAP[i].gpio;
    if (PIN_MAP[i].isPwm) {
      uint8_t ch = pwmChannelIndex++;
      pinToPwmChannel[gpio] = ch;
      ledcSetup(ch, PWM_FREQ, PWM_RESOLUTION);
      ledcAttachPin(gpio, ch);
      Serial.printf("[GPIO] PWM  pin %d (%s) -> channel %d\n",
                    gpio, PIN_MAP[i].label, ch);
    } else {
      pinMode(gpio, OUTPUT);
      digitalWrite(gpio, LOW);
      Serial.printf("[GPIO] DOUT pin %d (%s)\n", gpio, PIN_MAP[i].label);
    }
  }
}
