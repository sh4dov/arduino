# ESP32-C3 IoT Device Template (Reproducible Guide)

This document is a **reproducible, from-scratch guide** for creating an ESP-IDF (ESP32-C3) project that provides:

- **One-time provisioning** via an open AP + a configuration web form
- **Station (STA) mode** connect using stored credentials
- **Local web UI** (`/`) + **local API** (`GET /api/status`)
- **mDNS discovery** (`http://<device_name>.local/`)
- **Host notifications** via HTTP POST to `http://<host_ip>/api/motionSensor` (payload keys: `sensor`, `state`)

> Important: this template guide **does not include** `main/motion_sensor.c` (project-specific feature). The template describes how to wire any feature module (or a stub) into the status/UI + host POST pipeline.

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Create a new project (from scratch)](#2-create-a-new-project-from-scratch)
3. [Project structure](#3-project-structure)
4. [Core data types](#4-core-data-types)
5. [NVS Manager (`nvs_manager.h`, `nvs_manager.c`)](#5-nvs-manager-nvs_managerh-nvs_managerc)
6. [LED Indicator (`led_indicator.h`, `led_indicator.c`)](#6-led-indicator-led_indicatorh-led_indicatorc)
7. [Wi-Fi Manager (`wifi_manager.h`, `wifi_manager.c`)](#7-wi-fi-manager-wifi_managerh-wifi_managerc)
8. [mDNS Service (`mdns_service.h`, `mdns_service.c`)](#8-mdns-service-mdns_serviceh-mdns_servicec)
9. [Host Client (`host_client.h`, `host_client.c`)](#9-host-client-host_clienth-host_clientc)
10. [Web Server (`web_server.h`, `web_server.c`)](#10-web-server-web_serverh-web_serverc)
11. [Main application flow (`main.c`)](#11-main-application-flow-mainc)
12. [Template exclusion: `motion_sensor` and how to integrate your own feature](#12-template-exclusion-motion_sensor-and-how-to-integrate-your-own-feature)
13. [HTML samples (with required placeholder tokens)](#13-html-samples-with-required-placeholder-tokens)
14. [Clean-up: remove blink-example leftovers](#14-clean-up-remove-blink-example-leftovers)

---

## 1. Prerequisites

- **ESP-IDF** installed and working (this repo is built with ESP-IDF **v5.5.x**; see `dependencies.lock`).
- Basic familiarity with:
  - `idf.py set-target esp32c3`
  - `idf.py menuconfig` (optional)
  - `idf.py build flash monitor`

---

## 2. Create a new project (from scratch)

### Create the skeleton

Option A (recommended): create a new ESP-IDF project:

```bash
idf.py create-project esp32c3_iot_template
cd esp32c3_iot_template
```

Option B: copy this repo as a starting point and rename `project(...)` in `CMakeLists.txt`.

### Set the target

```bash
idf.py set-target esp32c3
```

### Add required dependencies (component manager)

Create `main/idf_component.yml`:

```yaml
dependencies:
  espressif/mdns: "^1.0.0"
```

Notes:

- **`cJSON`** comes from ESP-IDF’s built-in `json` component (no component-manager dependency needed).
- You do **not** need `led_strip` for this template (LED is controlled via GPIO in `led_indicator.c`).

### Ensure `CMakeLists.txt` files exist

Top-level `CMakeLists.txt` (minimal):

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(esp32c3_iot_template)
```

`main/CMakeLists.txt` (template version; **do not include `motion_sensor.c`**):

```cmake
idf_component_register(
  SRCS
    "main.c"
    "led_indicator.c"
    "nvs_manager.c"
    "wifi_manager.c"
    "web_server.c"
    "mdns_service.c"
    "host_client.c"
    "status_provider.c"    # optional tiny stub; see section 12
  INCLUDE_DIRS "."
  PRIV_REQUIRES
    nvs_flash
    esp_driver_gpio
    esp_system
    esp_wifi
    esp_netif
    esp_event
    esp_http_server
    esp_http_client
    json
    espressif__mdns
)
```

### Build, flash, monitor

```bash
idf.py build
idf.py -p <YOUR_SERIAL_PORT> flash monitor
```

---

## 3. Project structure

Recommended `main/` module layout:

- **Core app**
  - `main.c`
  - `app_state.h` (enums/shared types)
  - `status_provider.h/.c` (template stub or your feature’s interface)
- **Infrastructure modules**
  - `nvs_manager.h/.c` (persistent config)
  - `wifi_manager.h/.c` (AP + STA setup, event forwarding)
  - `web_server.h/.c` (provisioning UI + normal UI/API)
  - `mdns_service.h/.c` (mDNS `_http._tcp`)
  - `host_client.h/.c` (HTTP POST to host server with retry)
  - `led_indicator.h/.c` (LED patterns)

---

## 4. Core data types

### `app_mode_t` (in `app_state.h`)

Defines the application mode:

- `MODE_PROVISIONING`: AP is running and config portal is active
- `MODE_CONNECTING`: STA connection attempts are ongoing
- `MODE_NORMAL`: STA connected and device is “online”

### `motion_status_t` (in `app_state.h`)

Even without a motion sensor feature, we keep this enum because:

- `host_client_send_motion_update(motion_status_t status)` uses it
- `/api/status` returns a `status` integer derived from it

Recommended mapping:

- `MOTION_DETECTED` → `1`
- `MOTION_NONE` → `0`

### `app_config_t` (in `nvs_manager.h`)

Important correction vs older docs: `app_config_t` lives in **`nvs_manager.h`**, not in `app_state.h`.

Fields and lengths:

- `device_name` (max 31 chars + null terminator), used for hostname and mDNS
- `host_ip` IPv4 string (e.g. `192.168.1.50`)
- `wifi_ssid`
- `wifi_password`
- `provisioned` flag

---

## 5. NVS Manager (`nvs_manager.h`, `nvs_manager.c`)

Stores/loads configuration under a single namespace (e.g. `"storage"`).

### Public API

```c
esp_err_t nvs_manager_init(void);
esp_err_t nvs_manager_load_config(app_config_t *config);
esp_err_t nvs_manager_save_config(const app_config_t *config);
esp_err_t nvs_manager_factory_reset(void);
```

### Stored keys (recommended)

- `device_name` (string)
- `host_ip` (string)
- `wifi_ssid` (string)
- `wifi_pass` (string)
- `provisioned` (int32)

---

## 6. LED Indicator (`led_indicator.h`, `led_indicator.c`)

Implements simple patterns via a FreeRTOS task and a queue.

### Patterns

- `LED_SLOW_BLINK`: provisioning
- `LED_FAST_BLINK`: connecting
- `LED_SOLID_OFF`: normal idle
- `LED_SOLID_ON`: feature-defined “active” state (optional)

---

## 7. Wi-Fi Manager (`wifi_manager.h`, `wifi_manager.c`)

Responsibilities:

- Initialize netif/event loop
- Create default STA/AP netifs
- Start AP or STA mode
- Register an internal handler and **forward events** to `main.c`

### Public API

```c
typedef void (*wifi_event_callback_t)(app_mode_t mode, esp_event_base_t event_base, int32_t event_id, void* event_data);

void wifi_manager_init(wifi_event_callback_t cb, const char* device_name);
esp_err_t wifi_manager_start_ap(const char* ssid);
esp_err_t wifi_manager_start_sta(const char* ssid, const char* password);
esp_err_t wifi_manager_stop(void);
```

### Hostname + TX power notes

- If `device_name` is provided, set hostname on both STA and AP netifs.
- ESP32-C3 stability tip: call `esp_wifi_set_max_tx_power(40)` (20 dBm).

### STA authmode note

Some implementations set `wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK` (minimum security). If you need to support open/WPA networks, adjust this threshold accordingly.

---

## 8. mDNS Service (`mdns_service.h`, `mdns_service.c`)

Starts/stops mDNS advertising after STA is connected.

### Public API

```c
esp_err_t mdns_service_init(const char* device_name);
void mdns_service_stop(void);
```

Expected behavior:

- Hostname: `device_name`
- Service: `_http._tcp` port 80
- Discovery URL: `http://<device_name>.local/`

---

## 9. Host Client (`host_client.h`, `host_client.c`)

Sends HTTP POST updates to the configured host server.

### Contract (kept unchanged)

- **URL**: `http://<host_ip>/api/motionSensor`
- **Method**: POST
- **Payload**:

```json
{
  "sensor": "YourDeviceName",
  "state": 0
}
```

### Retry behavior

- Up to **3 attempts**
- **5 seconds** between attempts
- Non-blocking: implemented as a FreeRTOS task

---

## 10. Web Server (`web_server.h`, `web_server.c`)

Implements different endpoints depending on `g_app_mode`.

### Public API

```c
esp_err_t web_server_start(void);
esp_err_t web_server_stop(void);
```

### Provisioning mode

- `GET /`: serves provisioning form
- `POST /save`: parses `application/x-www-form-urlencoded`, validates inputs, saves config to NVS, returns success page, restarts device

Recommended validations (server-side):

- **Device name**: 1–31 chars, alnum + hyphen, cannot start/end with hyphen
- **Host IP**: strict IPv4 dotted-quad
- **Wi-Fi SSID**: non-empty

### Normal mode

- `GET /`: status page
- `GET /api/status`: JSON with device name and status integer (0/1)

Example response:

```json
{"name":"Kitchen-Sensor","status":0}
```

---

## 11. Main application flow (`main.c`)

High-level boot flow:

0. Define your provisioning AP name (example): `#define PROV_AP_SSID "new-motion-sensor"`
1. Initialize defaults for `g_app_config`
2. `nvs_manager_init()`
3. `led_indicator_init()`
4. `nvs_manager_load_config(&g_app_config)`
5. Decide:
   - **Provisioning** if config is missing/invalid
   - **Connecting** if config is valid + provisioned
6. `wifi_manager_init(on_wifi_event, hostname_or_NULL)`
7. Start AP or STA
8. LED pattern depends on mode

Wi-Fi callback (`on_wifi_event`) should implement:

- `WIFI_EVENT_AP_START`:
  - set `g_app_mode = MODE_PROVISIONING`
  - start provisioning web server
- `WIFI_EVENT_STA_START`:
  - set `MODE_CONNECTING`, fast blink
- `IP_EVENT_STA_GOT_IP`:
  - set `MODE_NORMAL`, LED off
  - start mDNS
  - stop provisioning web server (if any), start normal web server
- `WIFI_EVENT_STA_DISCONNECTED`:
  - set `MODE_CONNECTING`, fast blink
  - stop mDNS
  - call `esp_wifi_connect()` to retry

---

## 12. Template exclusion: `motion_sensor` and how to integrate your own feature

This template guide intentionally **excludes** `main/motion_sensor.c` / `main/motion_sensor.h`.

To keep the rest of the template functional, you have two options:

### Option A: Add a tiny `status_provider` stub (template-friendly)

Create `status_provider.h`:

```c
#pragma once
#include "app_state.h"
motion_status_t status_provider_get_status(void);
void status_provider_set_status(motion_status_t status);
```

Create `status_provider.c`:

```c
#include "status_provider.h"
static motion_status_t s_status = MOTION_NONE;
motion_status_t status_provider_get_status(void) { return s_status; }
void status_provider_set_status(motion_status_t status) { s_status = status; }
```

Then in your feature module (button, PIR, BLE event, etc.), call:

```c
status_provider_set_status(MOTION_DETECTED);
host_client_send_motion_update(MOTION_DETECTED);
```

### Option B: Your feature module is the provider

Expose `your_feature_get_status()` and have `web_server.c` call it.

---

## 13. HTML samples (with required placeholder tokens)

These HTML samples use **`%%...%%` tokens** that you can replace at runtime (server-side).

> Note on C string embedding: if you embed HTML in a `snprintf` format string, literal `%` must be written as `%%` (e.g., `width: 100%%;`). The `%%PLACEHOLDER%%` tokens below are shown as **raw HTML** tokens (not `snprintf` format strings).

### Provisioning page (`GET /`)

Required tokens:

- `%%CURRENT_DEVICE_NAME%%`
- `%%CURRENT_HOST_IP%%`

```html
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32-C3 Device Setup</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f4; color: #333; }
    .container { background-color: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); max-width: 500px; margin: auto; }
    h1 { color: #0056b3; text-align: center; margin-bottom: 25px; }
    label { display: block; margin-bottom: 8px; font-weight: bold; }
    input[type="text"], input[type="password"] { width: calc(100% - 22px); padding: 10px; margin-bottom: 15px; border: 1px solid #ddd; border-radius: 4px; font-size: 16px; }
    input[type="submit"] { background-color: #28a745; color: white; padding: 12px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; }
    input[type="submit"]:hover { background-color: #218838; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Device Setup</h1>
    <form action="/save" method="post">
      <label for="device_name">Device Name:</label>
      <input type="text" id="device_name" name="device_name" value="%%CURRENT_DEVICE_NAME%%" required>

      <label for="host_ip">Host Server IP:</label>
      <input type="text" id="host_ip" name="host_ip" value="%%CURRENT_HOST_IP%%"
             pattern="^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$" required>

      <label for="wifi_ssid">Wi-Fi SSID:</label>
      <input type="text" id="wifi_ssid" name="wifi_ssid" required>

      <label for="wifi_password">Wi-Fi Password:</label>
      <input type="password" id="wifi_password" name="wifi_password">

      <input type="submit" value="Save Configuration">
    </form>
  </div>
</body>
</html>
```

### Normal status page (`GET /`)

Required tokens:

- `%%DEVICE_NAME%%`
- `%%STATUS_CLASS%%` (`status-detected` or `status-none`)
- `%%MOTION_STATUS_TEXT%%` (`Detected` or `None`)

```html
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta http-equiv="refresh" content="5">
  <title>Device Status</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f4; color: #333; display: flex; justify-content: center; align-items: center; height: 90vh; }
    .container { background-color: #fff; padding: 40px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); text-align: center; }
    h1 { color: #0056b3; margin-bottom: 15px; }
    p { font-size: 24px; }
    .status-detected { color: #d9534f; font-weight: bold; }
    .status-none { color: #5cb85c; font-weight: bold; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Device: %%DEVICE_NAME%%</h1>
    <p>Status: <span class="%%STATUS_CLASS%%">%%MOTION_STATUS_TEXT%%</span></p>
  </div>
</body>
</html>
```

---

## 14. Clean-up: remove blink-example leftovers

If you started from an ESP-IDF example (like Blink), you may have leftover files/config unrelated to this template:

- `main/Kconfig.projbuild`: blink example menuconfig options (safe to delete for this template)
- Root `README.md`: blink example docs (replace with your own)
- `led_strip` dependency: not required (this template uses simple GPIO LED control)

Keeping them won’t necessarily break the build, but it **makes the project harder to understand** and can confuse future users of the template.
