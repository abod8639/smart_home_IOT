#include "mqtt_manager.h"
#include "globals.h"
#include "gpio_manager.h"
#include "http_handlers.h" // For executeIrSend if needed

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("\n[MQTT] Message arrived on topic: %s\n", topic);
  
  // Convert payload to String
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("  -> Payload: " + message);

  // Parse JSON
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, message);
  if (err) {
    Serial.print("  -> JSON Deserialization error: ");
    Serial.println(err.c_str());
    return;
  }

  // Route command based on action
  if (!doc["action"].is<const char*>()) {
    Serial.println("  -> Error: Missing 'action' field");
    return;
  }
  
  String action = doc["action"].as<String>();

  if (action == "get_state") {
    publishState();
  } 
  else if (action == "set_relay") {
    if (doc["pin"].is<int>() && doc["value"].is<int>()) {
      uint8_t gpio = doc["pin"].as<int>();
      int value = doc["value"].as<int>();
      writePin(gpio, value);
      
      // Publish event
      JsonDocument eventDoc;
      eventDoc["event"] = "relay_update";
      eventDoc["pin"] = gpio;
      eventDoc["value"] = digitalState[gpio] ? 1 : 0;
      publishEvent("relay_update", eventDoc);
      publishState();
    }
  }
  else if (action == "set_pwm") {
    if (doc["pin"].is<int>() && doc["value"].is<int>()) {
      uint8_t gpio = doc["pin"].as<int>();
      int value = constrain(doc["value"].as<int>(), 0, 255);
      writePin(gpio, value);
      
      // Publish event
      JsonDocument eventDoc;
      eventDoc["event"] = "pwm_update";
      eventDoc["pin"] = gpio;
      eventDoc["value"] = analogValue[gpio];
      publishEvent("pwm_update", eventDoc);
      publishState();
    }
  }
  else if (action == "control_ac") {
    if (doc["target_temp"].is<int>()) {
      targetTemperature = doc["target_temp"].as<int>();
      prefs.begin("pins", false);
      prefs.putInt("target_temp", targetTemperature);
      prefs.end();
    }
    if (doc["isOn"].is<bool>()) {
      bool isOn = doc["isOn"].as<bool>();
      writePin(19, isOn ? 1 : 0); // Assuming Relay 3 is AC
    }
    
    // Publish event
    JsonDocument eventDoc;
    eventDoc["event"] = "ac_update";
    eventDoc["isOn"] = digitalState[19] ? true : false;
    eventDoc["target_temperature"] = targetTemperature;
    publishEvent("ac_update", eventDoc);
    publishState();
  }
  else if (action == "set_ac_timer") {
    if (doc["seconds"].is<int>()) {
      int seconds = doc["seconds"].as<int>();
      if (seconds <= 0) {
        acTimerActive = false;
        acTimerDuration = 0;
        acTimerIrJson = "";
        
        prefs.begin("pins", false);
        prefs.putBool("ac_timer_active", false);
        prefs.end();
        Serial.println("[MQTT] AC Timer cancelled manually");
      } else if (doc["ir_code"].is<JsonObject>()) {
        acTimerDuration = (unsigned long)seconds * 1000;
        acTimerStartMillis = millis();
        acTimerActive = true;
        
        acTimerIrJson = "";
        serializeJson(doc["ir_code"], acTimerIrJson);
        
        prefs.begin("pins", false);
        prefs.putBool("ac_timer_active", true);
        prefs.putUInt("ac_timer_dur", acTimerDuration);
        prefs.putUInt("ac_timer_start", acTimerStartMillis); // Need to adjust this for reboots
        prefs.putString("ac_timer_json", acTimerIrJson);
        prefs.end();
        
        Serial.printf("[MQTT] AC Timer set for %d seconds\n", seconds);
      }
      publishState();
    }
  }
  else if (action == "ir_send") {
    String errorMsg;
    bool success = executeIrSend(doc, errorMsg);
    if (!success) {
      Serial.printf("[MQTT] IR Send failed: %s\n", errorMsg.c_str());
    } else {
      Serial.println("[MQTT] IR signal sent successfully");
    }
  }
  else if (action == "ir_learn") {
    // Cannot easily block inside MQTT callback.
    // In a real system, we'd trigger a flag to run learn in the main loop,
    // but for now, we'll try running it.
    // Note: This blocks MQTT client, might cause ping timeout.
    // Ideally use a task or flag.
  }
}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Attempting MQTT connection to ");
    Serial.print(MQTT_BROKER_IP);
    Serial.print("...");
    
    // Attempt to connect
    // Last Will and Testament
    if (mqttClient.connect(MQTT_DEVICE_ID, MQTT_TOPIC_STATUS, 1, true, "offline")) {
      Serial.println("connected");
      // Publish online status
      mqttClient.publish(MQTT_TOPIC_STATUS, "online", true);
      // Subscribe to command topic
      mqttClient.subscribe(MQTT_TOPIC_CMD);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setupMQTT() {
  mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
  mqttClient.setCallback(mqttCallback);
  // Optional: Increase buffer size if IR JSONs are large
  mqttClient.setBufferSize(2048);
}

void loopMQTT() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
}

void publishState() {
  if (!mqttClient.connected()) return;

  JsonDocument doc;
  
  // Populate digital pins
  JsonObject digitalObj = doc["digital"].to<JsonObject>();
  for (int i = 0; i < PIN_COUNT; i++) {
    if (!PIN_MAP[i].isPwm) {
      digitalObj[String(PIN_MAP[i].gpio)] = digitalState[PIN_MAP[i].gpio] ? 1 : 0;
    }
  }

  // Populate analog pins
  JsonObject analogObj = doc["analog"].to<JsonObject>();
  for (int i = 0; i < PIN_COUNT; i++) {
    if (PIN_MAP[i].isPwm) {
      analogObj[String(PIN_MAP[i].gpio)] = analogValue[PIN_MAP[i].gpio];
    }
  }

  // Populate AC state
  JsonObject acObj = doc["ac"].to<JsonObject>();
  acObj["target_temperature"] = targetTemperature;
  acObj["isOn"] = digitalState[19] ? true : false;
  
  // Timer
  acObj["timer_active"] = acTimerActive;
  if (acTimerActive) {
    unsigned long elapsed = millis() - acTimerStartMillis;
    if (elapsed < acTimerDuration) {
      acObj["timer_remaining"] = (acTimerDuration - elapsed) / 1000;
    } else {
      acObj["timer_remaining"] = 0;
    }
  }

  String output;
  serializeJson(doc, output);
  mqttClient.publish(MQTT_TOPIC_STATE, output.c_str(), true); // Retain state
}

void publishSensorData() {
  if (!mqttClient.connected()) return;
  if (isnan(currentTemp) || isnan(currentHum)) return;

  JsonDocument doc;
  doc["temperature"] = currentTemp;
  doc["humidity"] = currentHum;
  
  String output;
  serializeJson(doc, output);
  mqttClient.publish(MQTT_TOPIC_SENSOR, output.c_str(), true);
}

void publishEvent(const char* eventType, JsonDocument& payload) {
  if (!mqttClient.connected()) return;
  
  payload["event"] = eventType;
  String output;
  serializeJson(payload, output);
  mqttClient.publish(MQTT_TOPIC_EVENT, output.c_str());
}
