#include "http_handlers.h"
#include "globals.h"
#include <WiFi.h>

void sendJson(int code, JsonDocument& doc) {
  String body;
  serializeJson(doc, body);
  server.send(code, "application/json", body);
}

void sendSimple(int code, const char* status, const char* message) {
  JsonDocument doc;
  doc["status"]  = status;
  doc["message"] = message;
  sendJson(code, doc);
}

void setCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type,Accept");
}

void logRequest() {
  HTTPMethod m = server.method();
  // Skip noisy read-only requests
  if (m == HTTP_GET || m == HTTP_OPTIONS) return;

  String method = "";
  switch (m) {
    case HTTP_POST:    method = "POST";    break;
    case HTTP_DELETE:  method = "DELETE";  break;
    case HTTP_PUT:     method = "PUT";     break;
    case HTTP_PATCH:   method = "PATCH";   break;
    default:           method = "UNKNOWN"; break;
  }
  Serial.printf("\n[HTTP] %s %s\n", method.c_str(), server.uri().c_str());
  if (server.hasArg("plain")) {
    Serial.printf("  -> Body: %s\n", server.arg("plain").c_str());
  }
}

void handleOptions() {
  logRequest();
  setCorsHeaders();
  server.send(204);
}

void handleNotFound() {
  logRequest();
  setCorsHeaders();
  sendSimple(404, "error", "Endpoint not found");
}

void setupRoutes() {
  // CORS pre-flight
  server.onNotFound(handleNotFound);
  server.on("/",                HTTP_OPTIONS, handleOptions);
  server.on("/ping",            HTTP_OPTIONS, handleOptions);
  server.on("/sensors",         HTTP_OPTIONS, handleOptions);
  server.on("/control/digital", HTTP_OPTIONS, handleOptions);
  server.on("/control/analog",  HTTP_OPTIONS, handleOptions);
  server.on("/control/ac",      HTTP_OPTIONS, handleOptions);
  server.on("/control/ir/learn", HTTP_OPTIONS, handleOptions);
  server.on("/control/ir/send",  HTTP_OPTIONS, handleOptions);
  server.on("/ota/update",      HTTP_OPTIONS, handleOptions);
  server.on("/ota/status",      HTTP_OPTIONS, handleOptions);
  server.on("/system/info",     HTTP_OPTIONS, handleOptions);

  // Actual routes
  server.on("/ping",            HTTP_GET,  handlePing);
  server.on("/sensors",         HTTP_GET,  handleGetSensors);
  server.on("/control/digital", HTTP_POST, handleDigitalControl);
  server.on("/control/analog",  HTTP_POST, handleAnalogControl);
  server.on("/control/ac",      HTTP_POST, handleAcControl);
  server.on("/control/ir/learn", HTTP_GET,  handleIrLearn);
  server.on("/control/ir/send",  HTTP_POST, handleIrSend);
  server.on("/ota/update",      HTTP_POST, handleOtaUpdate);
  server.on("/ota/status",      HTTP_GET,  handleOtaStatus);
  server.on("/system/info",     HTTP_GET,  handleSystemInfo);
}
