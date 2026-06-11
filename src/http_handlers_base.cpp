#include "http_handlers.h"
#include "globals.h"
#include <WiFi.h>

void sendJson(int code, JsonDocument& doc) {
  server.setContentLength(measureJson(doc));
  server.send(code, "application/json", "");
  WiFiClient client = server.client();
  serializeJson(doc, client);
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

  const char* method = "UNKNOWN";
  switch (m) {
    case HTTP_POST:    method = "POST";    break;
    case HTTP_DELETE:  method = "DELETE";  break;
    case HTTP_PUT:     method = "PUT";     break;
    case HTTP_PATCH:   method = "PATCH";   break;
    default:           method = "UNKNOWN"; break;
  }
  Serial.printf("\n[HTTP] %s %s\n", method, server.uri().c_str());
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
  server.enableCORS(true);
  
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      setCorsHeaders();
      server.send(204);
    } else {
      handleNotFound();
    }
  });

  // Actual routes
  server.on("/ping",            HTTP_GET,  handlePing);
  server.on("/sensors",         HTTP_GET,  handleGetSensors);
  server.on("/control/digital", HTTP_POST, handleDigitalControl);
  server.on("/control/analog",  HTTP_POST, handleAnalogControl);
  server.on("/control/ac",      HTTP_POST, handleAcControl);
  server.on("/control/ac/timer", HTTP_POST, handleAcTimer);
  server.on("/control/ir/learn", HTTP_GET,  handleIrLearn);
  server.on("/control/ir/send",  HTTP_POST, handleIrSend);
  server.on("/ota/update",      HTTP_POST, handleOtaUpdate);
  server.on("/ota/status",      HTTP_GET,  handleOtaStatus);
  server.on("/system/info",     HTTP_GET,  handleSystemInfo);
}
