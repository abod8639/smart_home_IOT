#ifndef HTTP_HANDLERS_H
#define HTTP_HANDLERS_H

#include <ArduinoJson.h>

void setupRoutes();
void sendJson(int code, JsonDocument& doc);
void sendSimple(int code, const char* status, const char* message);
void setCorsHeaders();
void logRequest();

#endif // HTTP_HANDLERS_H
