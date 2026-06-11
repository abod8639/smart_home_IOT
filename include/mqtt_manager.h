#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

void setupMQTT();
void loopMQTT();
void publishState();
void publishSensorData();
void publishEvent(const char* eventType, JsonDocument& payload);

#endif // MQTT_MANAGER_H
