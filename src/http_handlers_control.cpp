#include "http_handlers.h"
#include "globals.h"
#include "gpio_manager.h"

void handleDigitalControl() {
  logRequest();
  setCorsHeaders();

  if (!server.hasArg("plain")) {
    Serial.println("  -> Error: Missing JSON body");
    sendSimple(400, "error", "Missing JSON body");
    return;
  }

  String body = server.arg("plain");
  Serial.print("  -> Body: ");
  Serial.println(body);

  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  if (err) {
    Serial.print("  -> JSON Deserialization error: ");
    Serial.println(err.c_str());
    sendSimple(400, "error", "Invalid JSON");
    return;
  }

  if (!req["pin"].is<int>() || !req["value"].is<int>()) {
    Serial.println("  -> Error: Missing pin or value parameter");
    sendSimple(400, "error", "Fields 'pin' and 'value' are required");
    return;
  }

  uint8_t gpio  = req["pin"].as<uint8_t>();
  int     value = req["value"].as<int>();

  Serial.printf("  -> Requesting Pin: %d, Value: %d\n", gpio, value);

  const PinConfig* pin = findPin(gpio);
  if (!pin) {
    Serial.printf("  -> Error: GPIO %d not in PIN_MAP\n", gpio);
    sendSimple(404, "error", "GPIO pin not registered in PIN_MAP");
    return;
  }

  if (pin->isPwm) {
    // Map digital value to PWM duty cycle (0 or 255)
    int pwmValue = (value != 0) ? 255 : 0;
    writePin(gpio, pwmValue);
    Serial.printf("  -> PWM Pin %d digitally set to %d\n", gpio, pwmValue);
  } else {
    writePin(gpio, value);
    Serial.printf("  -> Pin %d successfully set to %d\n", gpio, value);
  }

  JsonDocument res;
  res["status"]  = "ok";
  res["pin"]     = gpio;
  res["label"]   = pin->label;
  res["value"]   = digitalState[gpio] ? 1 : 0;
  sendJson(200, res);
}

void handleAnalogControl() {
  logRequest();
  setCorsHeaders();

  if (!server.hasArg("plain")) {
    Serial.println("  -> Error: Missing JSON body");
    sendSimple(400, "error", "Missing JSON body");
    return;
  }

  String body = server.arg("plain");
  Serial.print("  -> Body: ");
  Serial.println(body);

  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  if (err) {
    Serial.print("  -> JSON Deserialization error: ");
    Serial.println(err.c_str());
    sendSimple(400, "error", "Invalid JSON");
    return;
  }

  if (!req["pin"].is<int>() || !req["value"].is<int>()) {
    Serial.println("  -> Error: Missing pin or value parameter");
    sendSimple(400, "error", "Fields 'pin' and 'value' are required");
    return;
  }

  uint8_t gpio  = req["pin"].as<uint8_t>();
  int     value = constrain(req["value"].as<int>(), 0, 255);

  Serial.printf("  -> Requesting Pin: %d, PWM Value: %d\n", gpio, value);

  const PinConfig* pin = findPin(gpio);
  if (!pin) {
    Serial.printf("  -> Error: GPIO %d not in PIN_MAP\n", gpio);
    sendSimple(404, "error", "GPIO pin not registered in PIN_MAP");
    return;
  }

  if (!pin->isPwm) {
    Serial.println("  -> Error: Requested pin is configured as digital");
    sendSimple(400, "error", "This pin is digital. Use /control/digital instead");
    return;
  }

  writePin(gpio, value);

  JsonDocument res;
  res["status"] = "ok";
  res["pin"]    = gpio;
  res["label"]  = pin->label;
  res["value"]  = analogValue[gpio];
  sendJson(200, res);
}

void handleAcControl() {
  logRequest();
  setCorsHeaders();

  if (!server.hasArg("plain")) {
    Serial.println("  -> Error: Missing JSON body");
    sendSimple(400, "error", "Missing JSON body");
    return;
  }

  String body = server.arg("plain");
  Serial.print("  -> Body: ");
  Serial.println(body);

  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  if (err) {
    Serial.print("  -> JSON Deserialization error: ");
    Serial.println(err.c_str());
    sendSimple(400, "error", "Invalid JSON");
    return;
  }

  if (req["target_temp"].is<int>()) {
    targetTemperature = req["target_temp"].as<int>();
    Serial.printf("  -> Target Temperature set to: %d\n", targetTemperature);

    // Save to preferences
    prefs.begin("pins", false);
    prefs.putInt("target_temp", targetTemperature);
    prefs.end();
  }

  if (req["isOn"].is<bool>()) {
    bool isOn = req["isOn"].as<bool>();
    Serial.printf("  -> AC state set to: %s\n", isOn ? "ON" : "OFF");
    writePin(19, isOn ? 1 : 0);
  }

  JsonDocument res;
  res["status"]             = "ok";
  res["target_temperature"] = targetTemperature;
  res["isOn"]               = digitalState[19];
  sendJson(200, res);
}
