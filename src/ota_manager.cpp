#include "ota_manager.h"
#include "globals.h"
#include <ArduinoOTA.h>

void setupOTA() {
  // Hostname defaults to esp32-[MAC]
  ArduinoOTA.setHostname(DEVICE_NAME);

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }
      Serial.println("[ArduinoOTA] Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\n[ArduinoOTA] End");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("[ArduinoOTA] Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("[ArduinoOTA] Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("[ArduinoOTA] OTA Service Initialized");
}

void handleOTA() {
  ArduinoOTA.handle();
}
