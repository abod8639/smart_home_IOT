#include "http_handlers.h"
#include "globals.h"
#include <IRutils.h>

void handleIrLearn() {
  logRequest();
  setCorsHeaders();

  irrecv.resume(); // Clear any pending buffer
  delay(100);

  decode_results results;
  unsigned long start = millis();
  bool found = false;

  while (millis() - start < 10000) {
    if (irrecv.decode(&results)) {
      found = true;
      break;
    }
    delay(50);
  }

  if (found) {
    // Format the value as hex string
    char hexBuffer[32];
    sprintf(hexBuffer, "0x%llX", results.value);

    StaticJsonDocument<256> doc;
    doc["status"] = "ok";
    doc["protocol"] = typeToString(results.decode_type);
    doc["value"] = hexBuffer;
    doc["bits"] = results.bits;

    sendJson(200, doc);
    irrecv.resume();
  } else {
    sendSimple(408, "error", "Learning timeout - no signal detected");
  }
}

void handleIrSend() {
  logRequest();
  setCorsHeaders();

  if (!server.hasArg("plain")) {
    sendSimple(400, "error", "Missing JSON body");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> req;
  DeserializationError err = deserializeJson(req, body);
  if (err) {
    sendSimple(400, "error", "Invalid JSON");
    return;
  }

  if (!req.containsKey("protocol") || !req.containsKey("value") || !req.containsKey("bits")) {
    sendSimple(400, "error", "protocol, value, and bits fields are required");
    return;
  }

  String protocol = req["protocol"].as<String>();
  String valueStr = req["value"].as<String>();
  uint16_t bits   = req["bits"].as<uint16_t>();

  uint64_t val = strtoull(valueStr.c_str(), nullptr, 16);
  decode_type_t type = strToDecodeType(protocol.c_str());

  irrecv.disableIRIn();
  delay(50);
  irsend.send(type, val, bits);
  delay(50);
  irrecv.enableIRIn();

  StaticJsonDocument<128> res;
  res["status"] = "ok";
  res["message"] = "IR signal transmitted successfully";
  sendJson(200, res);
}
