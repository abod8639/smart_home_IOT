#include "http_handlers.h"
#include "globals.h"
#include <WiFi.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <Update.h>

void handleOtaUpdate() {
  logRequest();
  setCorsHeaders();

  if (otaState == OtaState::IN_PROGRESS) {
    sendSimple(409, "error", "OTA already in progress");
    return;
  }

  if (!server.hasArg("plain")) {
    sendSimple(400, "error", "Missing JSON body with 'url' field");
    return;
  }

  StaticJsonDocument<256> req;
  DeserializationError err = deserializeJson(req, server.arg("plain"));
  if (err || !req.containsKey("url")) {
    sendSimple(400, "error", "Invalid JSON — 'url' field required");
    return;
  }

  String firmwareUrl = req["url"].as<String>();
  if (!firmwareUrl.startsWith("http")) {
    sendSimple(400, "error", "URL must start with http:// or https://");
    return;
  }

  // Acknowledge immediately; OTA runs in background task
  sendSimple(202, "accepted", "OTA update started. Poll /ota/status for progress.");

  // Launch OTA in a FreeRTOS task to avoid blocking the web server
  static String urlBuffer;
  urlBuffer = firmwareUrl;

  xTaskCreatePinnedToCore(
    [](void* param) {
      String url = *((String*)param);

      otaState    = OtaState::IN_PROGRESS;
      otaProgress = 0;
      otaError    = "";

      Serial.printf("[OTA] Starting update from: %s\n", url.c_str());

      // Register progress callback
      Update.onProgress([](size_t done, size_t total) {
        if (total > 0) {
          otaProgress = (int)((done * 100) / total);
          Serial.printf("[OTA] Progress: %d%%\n", otaProgress);
        }
      });

      HTTPClient http;
      http.begin(url);
      int httpCode = http.GET();

      if (httpCode != HTTP_CODE_OK) {
        otaState = OtaState::FAILED;
        otaError = "HTTP error: " + String(httpCode);
        Serial.printf("[OTA] HTTP fetch failed: %d\n", httpCode);
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      int contentLength = http.getSize();
      if (contentLength <= 0) {
        otaState = OtaState::FAILED;
        otaError = "Invalid firmware content length";
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      if (!Update.begin(contentLength, U_FLASH)) {
        otaState = OtaState::FAILED;
        otaError = "OTA begin failed: not enough flash space";
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      WiFiClient* stream    = http.getStreamPtr();
      size_t      written   = Update.writeStream(*stream);

      if (written != (size_t)contentLength) {
        otaState = OtaState::FAILED;
        otaError = "Write mismatch: expected " + String(contentLength) +
                   ", wrote " + String(written);
        Update.abort();
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      if (!Update.end()) {
        otaState = OtaState::FAILED;
        otaError = "OTA end failed: " + String(Update.getError());
        http.end();
        vTaskDelete(nullptr);
        return;
      }

      http.end();
      otaProgress = 100;
      otaState    = OtaState::SUCCESS;

      Serial.println("[OTA] Update complete! Rebooting in 2 seconds...");
      delay(2000);
      ESP.restart();

      vTaskDelete(nullptr);
    },
    "ota_task",
    8192,       // 8KB stack — enough for HTTP + flash
    &urlBuffer,
    1,          // priority
    nullptr,
    0           // run on core 0 (loop runs on core 1)
  );
}

void handleOtaStatus() {
  logRequest();
  setCorsHeaders();

  StaticJsonDocument<128> doc;

  switch (otaState) {
    case OtaState::IDLE:
      doc["state"]    = "idle";
      doc["progress"] = 0;
      break;
    case OtaState::IN_PROGRESS:
      doc["state"]    = "in_progress";
      doc["progress"] = otaProgress;
      break;
    case OtaState::SUCCESS:
      doc["state"]    = "success";
      doc["progress"] = 100;
      break;
    case OtaState::FAILED:
      doc["state"]    = "failed";
      doc["progress"] = otaProgress;
      doc["error"]    = otaError;
      break;
  }

  sendJson(200, doc);
}
