# ESP32 Smart Home IoT Firmware

This repository contains the main firmware for the ESP32-based Smart Home IoT node. The project is built using PlatformIO and the Arduino framework. It integrates local control APIs (HTTP REST), infrared (IR) signal learning and transmission, non-volatile state persistence, and Matter-compliant smart home endpoints (OnOff, Dimmable, and Color light controls).

## Key Features

- **Matter Smart Home Integration**: Exposes 6 distinct Matter endpoints (lights, relays, RGB controller) configurable via BLE commissioning.
- **RESTful Control API**: Lightweight HTTP server providing routes to retrieve sensor data, write to GPIOs, capture/replay IR codes, and trigger OTA updates.
- **IR learning & Emission**: Supports a wide array of IR protocols (NEC, SAMSUNG, SONY, etc.) and raw pulse-width duration signals to learn and replay remote controller commands.
- **Non-Volatile Storage (NVS) Persistence**: Automatically stores GPIO and AC states in NVS to restore them on power cycles.
- **Over-The-Air (OTA) Updates**: Asynchronous firmware flashing from a remote HTTP URL executing in a dedicated FreeRTOS task.
- **Sensor Monitoring**: Periodic readings of environmental parameters via a DHT22 temperature and humidity sensor.

## Hardware Configuration & Pin Mapping

The pinouts and peripheral configurations are defined dynamically inside [globals.h](file:///home/dexter/Documents/PlatformIO/Projects/smart_home_IOT/include/globals.h) and managed by [gpio_manager.cpp](file:///home/dexter/Documents/PlatformIO/Projects/smart_home_IOT/src/gpio_manager.cpp).

| GPIO | Peripheral / Label | Type | Description |
|---|---|---|---|
| 0 | BOOT Button | Input | Factory reset and decommissioning (Hold for 5 seconds) |
| 2 | relay_1 | Digital Output | Controls relay 1 (e.g., main lamp) |
| 4 | DHT22 | Input | Temperature and humidity sensor data line |
| 18 | relay_2 | Digital Output | Controls relay 2 (e.g., door lock) |
| 19 | relay_3 | Digital Output | Controls relay 3 (e.g., AC power) |
| 21 | relay_4 | Digital Output | Controls relay 4 (spare output) |
| 22 | pwm_lamp | PWM Output | Dimmable lamp controller (8-bit resolution, 5kHz) |
| 23 | pwm_rgb_r | PWM Output | RGB LED Red channel |
| 25 | pwm_rgb_g | PWM Output | RGB LED Green channel |
| 26 | pwm_rgb_b | PWM Output | RGB LED Blue channel |
| 32 | IR Receiver | Input (Pullup) | Captured signal input line |
| 33 | IR Transmitter | Output | Emitted carrier signal line (38 kHz default) |

## Matter Integration

The device boots into Matter mode by initializing endpoints defined in [main.cpp](file:///home/dexter/Documents/PlatformIO/Projects/smart_home_IOT/src/main.cpp).

- **Endpoints**:
  - **Endpoint 1**: Relay 1 (Main Lamp) - GPIO 2 (OnOff)
  - **Endpoint 2**: Relay 2 (Door Lock) - GPIO 18 (OnOff)
  - **Endpoint 3**: Relay 3 (AC Power) - GPIO 19 (OnOff)
  - **Endpoint 4**: Relay 4 (Spare Relay) - GPIO 21 (OnOff)
  - **Endpoint 5**: PWM Dimmable Lamp - GPIO 22 (Dimmable)
  - **Endpoint 6**: RGB LED - GPIO 23, 25, 26 (Color Light)
- **Commissioning**:
  - The stack advertises over Bluetooth Low Energy (BLE).
  - Default SSID: `SmartHome-ESP32`
  - Default Passcode: `20202021`
- **Decommissioning / Factory Reset**:
  - To clear credentials and put the device back into pairing mode, hold the BOOT button (GPIO 0) for 5 seconds. The device will decommission itself and restart.

## REST API Reference

The web server routes are initialized via `setupRoutes` in [http_handlers_base.cpp](file:///home/dexter/Documents/PlatformIO/Projects/smart_home_IOT/src/http_handlers_base.cpp).

### 1. Ping Check
- **Endpoint**: `/ping`
- **Method**: `GET`
- **Response**:
  ```json
  {
    "status": "ok",
    "device": "SmartHome-ESP32",
    "version": "1.0.0",
    "uptime": 120
  }
  ```

### 2. Environmental Sensors
- **Endpoint**: `/sensors`
- **Method**: `GET`
- **Response**:
  ```json
  {
    "temperature": 24.5,
    "humidity": 58.2,
    "pins": {
      "relay_1": 0,
      "relay_2": 1,
      "relay_3": 0,
      "relay_4": 0,
      "pwm_lamp": 128,
      "pwm_rgb_r": 0,
      "pwm_rgb_g": 255,
      "pwm_rgb_b": 100
    },
    "wifi_rssi": -62,
    "heap_free": 184520,
    "target_temperature": 24
  }
  ```

### 3. Digital Output Control
- **Endpoint**: `/control/digital`
- **Method**: `POST`
- **Request Body**:
  ```json
  {
    "pin": 2,
    "value": 1
  }
  ```
- **Response**:
  ```json
  {
    "status": "ok",
    "pin": 2,
    "label": "relay_1",
    "value": 1
  }
  ```

### 4. Analog Output Control (PWM)
- **Endpoint**: `/control/analog`
- **Method**: `POST`
- **Request Body**:
  ```json
  {
    "pin": 22,
    "value": 128
  }
  ```
- **Response**:
  ```json
  {
    "status": "ok",
    "pin": 22,
    "label": "pwm_lamp",
    "value": 128
  }
  ```

### 5. AC Parameter Control
- **Endpoint**: `/control/ac`
- **Method**: `POST`
- **Request Body**:
  ```json
  {
    "target_temp": 22,
    "isOn": true
  }
  ```
- **Response**:
  ```json
  {
    "status": "ok",
    "target_temperature": 22,
    "isOn": true
  }
  ```

### 6. IR Code Learning
- **Endpoint**: `/control/ir/learn`
- **Method**: `GET`
- **Description**: Listens for an incoming IR signal for up to 10 seconds.
- **Response (Protocol Encoded)**:
  ```json
  {
    "status": "ok",
    "protocol": "NEC",
    "value": "0x1FE48B7",
    "bits": 32,
    "address": 0,
    "command": 72,
    "rawData": 1234567,
    "frequency": 38
  }
  ```
- **Response (Raw Timings)**:
  ```json
  {
    "status": "ok",
    "protocol": "RAW",
    "value": "9020,4480,560,560,560,1680...",
    "bits": 68,
    "frequency": 38
  }
  ```

### 7. IR Code Transmission
- **Endpoint**: `/control/ir/send`
- **Method**: `POST`
- **Request Body (Protocol Encoded)**:
  ```json
  {
    "protocol": "NEC",
    "value": "0x1FE48B7",
    "bits": 32,
    "address": 0,
    "command": 72
  }
  ```
- **Request Body (Raw Timings)**:
  ```json
  {
    "protocol": "RAW",
    "value": "9020,4480,560,560,560,1680",
    "bits": 6,
    "frequency": 38
  }
  ```
- **Response**:
  ```json
  {
    "status": "ok",
    "message": "IR signal transmitted successfully"
  }
  ```

### 8. OTA Firmware Update
- **Endpoint**: `/ota/update`
- **Method**: `POST`
- **Request Body**:
  ```json
  {
    "url": "http://192.168.1.100/firmware.bin"
  }
  ```
- **Response**:
  ```json
  {
    "status": "accepted",
    "message": "OTA update started. Poll /ota/status for progress."
  }
  ```

### 9. OTA Status Polling
- **Endpoint**: `/ota/status`
- **Method**: `GET`
- **Response**:
  ```json
  {
    "state": "in_progress",
    "progress": 45
  }
  ```

### 10. System Information
- **Endpoint**: `/system/info`
- **Method**: `GET`
- **Response**:
  ```json
  {
    "firmware": "1.0.0",
    "device": "SmartHome-ESP32",
    "chip_model": "ESP32-D0WDQ6",
    "chip_cores": 2,
    "cpu_mhz": 240,
    "flash_mb": 4,
    "heap_total": 298124,
    "heap_free": 182390,
    "uptime_s": 240,
    "ip_address": "192.168.1.50",
    "mac": "AA:BB:CC:DD:EE:FF"
  }
  ```

## Compile and Upload

This project is built using PlatformIO. The primary configuration details are specified in [platformio.ini](file:///home/dexter/Documents/PlatformIO/Projects/smart_home_IOT/platformio.ini).

### Prerequisites
1. Install [PlatformIO Core](https://platformio.org/platformio-ide) or use the PlatformIO IDE extension in VS Code.
2. An ESP32 development board (e.g., NodeMCU ESP32, ESP32 DevKit v1).

### Compilation
To compile the firmware, execute:
```bash
pio run
```

### Uploading via USB (Default)
By default, the platform configuration uses the `esptool` protocol. Ensure the serial port is configured correctly in [platformio.ini](file:///home/dexter/Documents/PlatformIO/Projects/smart_home_IOT/platformio.ini) (`upload_port` parameter).
To flash the firmware, run:
```bash
pio run --target upload
```

### Partition Table
This project uses a custom partition layout optimized for maximum application space and OTA support. The partitions are defined in [max_ota.csv](file:///home/dexter/Documents/PlatformIO/Projects/smart_home_IOT/max_ota.csv):
- **NVS Partition**: 16KB for storing runtime state and Matter settings.
- **OTA Data Partition**: 8KB.
- **App 0 Partition**: 1.9MB (0x1F0000) for active application execution.
- **App 1 Partition**: 1.9MB (0x1F0000) for flashing incoming updates.
