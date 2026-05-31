#include "http_handlers.h"
#include "globals.h"

// Helper functions for parsing
int parseCommaSeparatedHex(const char* str, IRRawDataType* outArray, int maxElements) {
  char* temp = strdup(str);
  if (!temp) return 0;
  char* token = strtok(temp, ",");
  int count = 0;
  while (token != nullptr && count < maxElements) {
    outArray[count++] = (IRRawDataType)strtoull(token, nullptr, 16);
    token = strtok(nullptr, ",");
  }
  free(temp);
  return count;
}

int parseCommaSeparatedDec(const char* str, uint16_t* outArray, int maxElements) {
  char* temp = strdup(str);
  if (!temp) return 0;
  char* token = strtok(temp, ",");
  int count = 0;
  while (token != nullptr && count < maxElements) {
    outArray[count++] = (uint16_t)strtoul(token, nullptr, 10);
    token = strtok(nullptr, ",");
  }
  free(temp);
  return count;
}

decode_type_t parseProtocol(const char* name) {
  if (strcasecmp(name, "NEC") == 0) return NEC;
  if (strcasecmp(name, "SAMSUNG") == 0) return SAMSUNG;
  if (strcasecmp(name, "SONY") == 0) return SONY;
  if (strcasecmp(name, "PANASONIC") == 0) return PANASONIC;
  if (strcasecmp(name, "DENON") == 0) return DENON;
  if (strcasecmp(name, "SHARP") == 0) return SHARP;
  if (strcasecmp(name, "JVC") == 0) return JVC;
  if (strcasecmp(name, "RC5") == 0) return RC5;
  if (strcasecmp(name, "RC6") == 0) return RC6;
  if (strcasecmp(name, "LG") == 0) return LG;
  if (strcasecmp(name, "PULSE_DISTANCE") == 0) return PULSE_DISTANCE;
  if (strcasecmp(name, "PULSE_WIDTH") == 0) return PULSE_WIDTH;
  return UNKNOWN;
}

void handleIrLearn() {
  logRequest();
  setCorsHeaders();

  IrReceiver.resume(); // Clear any pending buffer
  delay(100);

  unsigned long start = millis();
  bool found = false;

  while (millis() - start < 10000) {
    if (IrReceiver.decode()) {
      found = true;
      break;
    }
    delay(50);
  }

  if (found) {
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      // Unrecognized protocol (like custom AC or noise) -> Store as RAW timings
      String rawDataStr = "";
      for (int i = 1; i < IrReceiver.decodedIRData.rawlen; i++) {
        uint32_t duration = IrReceiver.decodedIRData.rawDataPtr->rawbuf[i] * MICROS_PER_TICK;
        rawDataStr += String(duration);
        if (i < IrReceiver.decodedIRData.rawlen - 1) {
          rawDataStr += ",";
        }
      }

      Serial.printf("\n[IR] RAW signal captured (%d samples)\n", IrReceiver.decodedIRData.rawlen - 1);
      Serial.printf("  -> %s\n", rawDataStr.c_str());

      JsonDocument doc;
      doc["status"]   = "ok";
      doc["protocol"] = "RAW";
      doc["value"]    = rawDataStr;
      doc["bits"]     = IrReceiver.decodedIRData.rawlen - 1;

      sendJson(200, doc);
    } else {
      // Standard protocol (NEC, Samsung, Sony, PULSE_DISTANCE, PULSE_WIDTH etc.) -> Store decoded values
      String protocolName = getProtocolString(IrReceiver.decodedIRData.protocol);
      
      int bitsPerWord = sizeof(IRRawDataType) * 8;
      int numWords = (IrReceiver.decodedIRData.numberOfBits + bitsPerWord - 1) / bitsPerWord;
      String hexStr = "";
      for (int w = 0; w < numWords; w++) {
        char valBuf[32];
        snprintf(valBuf, sizeof(valBuf), "0x%llX", (unsigned long long)IrReceiver.decodedIRData.decodedRawDataArray[w]);
        hexStr += valBuf;
        if (w < numWords - 1) {
          hexStr += ",";
        }
      }

      JsonDocument doc;
      doc["status"]   = "ok";
      doc["protocol"] = protocolName;
      doc["value"]    = hexStr;
      doc["bits"]     = IrReceiver.decodedIRData.numberOfBits;
      doc["address"]  = IrReceiver.decodedIRData.address;
      doc["command"]  = IrReceiver.decodedIRData.command;
      doc["rawData"]  = (uint32_t)IrReceiver.decodedIRData.decodedRawData;

      // Extract specific timings for custom/universal Pulse Distance or Width protocols
      if (IrReceiver.decodedIRData.protocol == PULSE_DISTANCE || IrReceiver.decodedIRData.protocol == PULSE_WIDTH) {
        doc["headerMark"]  = IrReceiver.decodedIRData.DistanceWidthTimingInfo.HeaderMarkMicros;
        doc["headerSpace"] = IrReceiver.decodedIRData.DistanceWidthTimingInfo.HeaderSpaceMicros;
        doc["oneMark"]     = IrReceiver.decodedIRData.DistanceWidthTimingInfo.OneMarkMicros;
        doc["oneSpace"]    = IrReceiver.decodedIRData.DistanceWidthTimingInfo.OneSpaceMicros;
        doc["zeroMark"]    = IrReceiver.decodedIRData.DistanceWidthTimingInfo.ZeroMarkMicros;
        doc["zeroSpace"]   = IrReceiver.decodedIRData.DistanceWidthTimingInfo.ZeroSpaceMicros;
        
        bool isMsb = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_MSB_FIRST) != 0;
        doc["flags"]       = isMsb ? PROTOCOL_IS_MSB_FIRST : PROTOCOL_IS_LSB_FIRST;
        doc["isMsb"]       = isMsb;
      }

      Serial.printf("\n[IR] Protocol : %s  (%d bits)\n", protocolName.c_str(), IrReceiver.decodedIRData.numberOfBits);
      Serial.printf("  -> Value: %s\n", hexStr.c_str());
      if (IrReceiver.decodedIRData.protocol == PULSE_DISTANCE || IrReceiver.decodedIRData.protocol == PULSE_WIDTH) {
        Serial.printf("  -> Header: %u / %u  One: %u / %u  Zero: %u / %u\n",
          IrReceiver.decodedIRData.DistanceWidthTimingInfo.HeaderMarkMicros,
          IrReceiver.decodedIRData.DistanceWidthTimingInfo.HeaderSpaceMicros,
          IrReceiver.decodedIRData.DistanceWidthTimingInfo.OneMarkMicros,
          IrReceiver.decodedIRData.DistanceWidthTimingInfo.OneSpaceMicros,
          IrReceiver.decodedIRData.DistanceWidthTimingInfo.ZeroMarkMicros,
          IrReceiver.decodedIRData.DistanceWidthTimingInfo.ZeroSpaceMicros);
      }

      sendJson(200, doc);
    }
    IrReceiver.resume();
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
  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  if (err) {
    sendSimple(400, "error", "Invalid JSON");
    return;
  }

  if (!req["protocol"].is<const char*>() || !req["value"].is<const char*>() || !req["bits"].is<int>()) {
    sendSimple(400, "error", "protocol, value, and bits fields are required");
    return;
  }

  String protocol = req["protocol"].as<String>();
  String valueStr = req["value"].as<String>();
  uint16_t bits   = req["bits"].as<uint16_t>();

  // Disable receiver before sending
  IrReceiver.stop();
  delay(50);

  Serial.printf("[IR] Preparing to send on GPIO%d\n", IR_SEND_PIN);

  if (protocol.equalsIgnoreCase("RAW")) {
    // Parse decimal durations
    uint16_t* rawArray = new uint16_t[bits];
    if (rawArray) {
      int parsedCount = parseCommaSeparatedDec(valueStr.c_str(), rawArray, bits);
      IrSender.sendRaw(rawArray, parsedCount, 38);
      delete[] rawArray;
    }
  } else if (protocol.equalsIgnoreCase("PULSE_DISTANCE") || 
             protocol.equalsIgnoreCase("PULSE_WIDTH")) {
    
    // kHz frequency — default 38 kHz
    uint8_t frequency = req["frequency"].is<uint8_t>() ? req["frequency"].as<uint8_t>() : 38;

    // Timing parameters (µs) with defaults from observed AC remote
    uint16_t headerMark  = req["headerMark"].is<uint16_t>()  ? req["headerMark"].as<uint16_t>()  : 3800;
    uint16_t headerSpace = req["headerSpace"].is<uint16_t>() ? req["headerSpace"].as<uint16_t>() : 1950;
    uint16_t oneMark     = req["oneMark"].is<uint16_t>()     ? req["oneMark"].as<uint16_t>()     : 400;
    uint16_t oneSpace    = req["oneSpace"].is<uint16_t>()    ? req["oneSpace"].as<uint16_t>()    : 1500;
    uint16_t zeroMark    = req["zeroMark"].is<uint16_t>()    ? req["zeroMark"].as<uint16_t>()    : 400;
    uint16_t zeroSpace   = req["zeroSpace"].is<uint16_t>()   ? req["zeroSpace"].as<uint16_t>()   : 550;
    
    uint8_t flags = PROTOCOL_IS_LSB_FIRST;
    if (req["isMsb"].is<bool>() && req["isMsb"].as<bool>()) {
      flags = PROTOCOL_IS_MSB_FIRST;
    }

    IRRawDataType dataArray[RAW_DATA_ARRAY_SIZE] = {0};
    int parsedCount = parseCommaSeparatedHex(valueStr.c_str(), dataArray, RAW_DATA_ARRAY_SIZE);
    (void)parsedCount;

    Serial.printf("[IR] Sending PULSE_DISTANCE: freq=%u kHz, header=%u/%u, one=%u/%u, zero=%u/%u, bits=%u, flags=%u\n",
      frequency, headerMark, headerSpace, oneMark, oneSpace, zeroMark, zeroSpace, bits, flags);

    IrSender.sendPulseDistanceWidthFromArray(
      frequency, headerMark, headerSpace, oneMark, oneSpace, zeroMark, zeroSpace,
      dataArray, bits, flags, 0, 0
    );
  } else {
    // Standard protocols: NEC, Samsung, Sony, LG, etc.
    // The Flutter app sends address + command if available (simpler and more reliable)
    // Fallback: reconstruct IRData from hex value array
    IRData irData;
    memset(&irData, 0, sizeof(irData));
    irData.protocol     = parseProtocol(protocol.c_str());
    irData.numberOfBits = bits;

    // Prefer address/command if explicitly sent
    if (req["address"].is<uint16_t>() && req["command"].is<uint16_t>()) {
      irData.address = req["address"].as<uint16_t>();
      irData.command = req["command"].as<uint16_t>();
      irData.decodedRawData = req["rawData"].is<uint32_t>()
                              ? req["rawData"].as<uint32_t>()
                              : 0;
      irData.decodedRawDataArray[0] = irData.decodedRawData;
    } else {
      // Reconstruct from hex word array
      int bitsPerWord = sizeof(IRRawDataType) * 8;
      int numWords = (bits + bitsPerWord - 1) / bitsPerWord;
      IRRawDataType dataArray[RAW_DATA_ARRAY_SIZE] = {0};
      parseCommaSeparatedHex(valueStr.c_str(), dataArray, RAW_DATA_ARRAY_SIZE);
      for (int w = 0; w < numWords && w < RAW_DATA_ARRAY_SIZE; w++) {
        irData.decodedRawDataArray[w] = dataArray[w];
      }
      if (numWords == 1) {
        irData.decodedRawData = dataArray[0];
      }
    }

    Serial.printf("[IR] Sending %s: address=0x%X, command=0x%X, bits=%u\n",
      protocol.c_str(), irData.address, irData.command, bits);

    IrSender.write(&irData);
  }

  delay(50);
  IrReceiver.start(); // Re-enable receiver

  JsonDocument res;
  res["status"] = "ok";
  res["message"] = "IR signal transmitted successfully";
  sendJson(200, res);
}
