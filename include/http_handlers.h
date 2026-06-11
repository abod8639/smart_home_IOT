#ifndef HTTP_HANDLERS_H
#define HTTP_HANDLERS_H

#include <ArduinoJson.h>

// Route initialization
void setupRoutes();

// Utility response helpers
void sendJson(int code, JsonDocument& doc);
void sendSimple(int code, const char* status, const char* message);
void setCorsHeaders();
void logRequest();

// Base Handlers
void handleOptions();
void handleNotFound();

// System / General Handlers
void handlePing();
void handleGetSensors();
void handleSystemInfo();

// Pin Control Handlers
void handleDigitalControl();
void handleAnalogControl();
void handleAcControl();
void handleAcTimer();

// IR Handlers
void handleIrLearn();
void handleIrSend();
bool executeIrSend(JsonDocument& req, String& errorMsg);

// OTA Handlers
void handleOtaUpdate();
void handleOtaStatus();

#endif // HTTP_HANDLERS_H
