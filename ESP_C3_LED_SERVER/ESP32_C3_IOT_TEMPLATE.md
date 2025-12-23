# ESP32-C3 IoT Device Template (Reproducible Guide)

This document is a **reproducible, from-scratch guide** for creating an ESP-IDF (ESP32-C3) project that provides:

- **One-time provisioning** via an open AP + a configuration web form
- **Station (STA) mode** connect using stored credentials
- **Local web UI** (`/`) + **local API** (`GET /api/status`)
- **mDNS discovery** (`http://<device_name>.local/`)
- **Host notifications** via HTTP POST to `http://<host_addr>/api/motionSensor` (payload keys: `sensor`, `state`)
- **(Repo implementation)** Provisioning includes **Wi‑Fi scan UX** (`GET /scan_wifi`) and a cached list endpoint (`GET /get_cached_wifi_list`)
- **(Repo implementation)** Normal mode supports **editing config** (`GET /edit`, `POST /update`) and rebooting

> Note: the **template variant** may exclude `main/motion_sensor.c` (project-specific). This repo includes `motion_sensor.c` for the MH‑SR602 PIR on GPIO4 and uses it as the status provider for the UI/API and host POST updates.

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
13. [HTML approaches (repo: `snprintf` format strings; template: token replacement)](#13-html-approaches-repo-snprintf-format-strings-template-token-replacement)
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
- The LED in this template is controlled via GPIO in `led_indicator.c` (no LED-strip library required).

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

`main/CMakeLists.txt` (**this repo**, includes MH‑SR602 PIR support):

```cmake
idf_component_register(
  SRCS
    "main.c"
    "led_indicator.c"
    "motion_sensor.c"
    "nvs_manager.c"
    "wifi_manager.c"
    "web_server.c"
    "mdns_service.c"
    "host_client.c"
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
- **Feature module**
  - `motion_sensor.h/.c` (this repo) OR `status_provider.h/.c` (template stub / your feature’s interface)
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
- `host_addr` host **address** used by the HTTP client (this repo allows **IPv4, hostname, and optional `:port`**; max length is larger than an IPv4 string)
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
- `host_addr` (string)
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

Repo notes:

- **GPIO**: `GPIO8` (`LED_GPIO` in `led_indicator.c`)
- **Active-low**: the onboard LED uses inverted logic (**LOW = ON**, **HIGH = OFF**)
- **Timings**: slow blink \(1000 ms\), fast blink \(250 ms\)

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
esp_err_t wifi_manager_start_sta(const app_config_t *config);
esp_err_t wifi_manager_stop(void);
esp_err_t wifi_manager_scan_networks(void);
esp_err_t wifi_manager_scan_networks_async(void);
esp_err_t wifi_manager_get_cached_networks(scanned_wifi_ap_t **ap_records, uint16_t *ap_count);
```

### Hostname + TX power notes

- If `device_name` is provided, set hostname on both STA and AP netifs.
- ESP32-C3 stability tip: call `esp_wifi_set_max_tx_power(40)` (20 dBm).
  - This repo sets the AP hostname to `"ESP32-AP-Sensor"` when no device name is provided (i.e., during first-boot provisioning).

### Scanning + cache notes (repo behavior)

- **AP mode uses AP+STA** (`WIFI_MODE_APSTA`) so scans work while the provisioning AP is up.
- **Cache size**: keeps up to **20** APs (`DEFAULT_SCAN_LIST_SIZE`).
- **AP list access**: `wifi_manager_get_cached_networks()` returns a heap-allocated copy; caller must `free()`.

### STA authmode note

Some implementations set `wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK` (minimum security). If you need to support open/WPA networks, adjust this threshold accordingly.
This repo currently enforces `WIFI_AUTH_WPA2_PSK` as the minimum and enables WPA3 SAE PWE (`WPA3_SAE_PWE_BOTH`), which means **open networks won’t connect** unless you change that threshold.

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

Repo notes:

- If `mdns_service_init()` is called again, it will stop and re-initialize mDNS.
- Instance name is set to `"ESP32-C3 Motion Sensor"`.

---

## 9. Host Client (`host_client.h`, `host_client.c`)

Sends HTTP POST updates to the configured host server.

### Contract (kept unchanged)

- **URL**: `http://<host_addr>/api/motionSensor`
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

Repo notes:

- **Timeout**: 5 seconds per HTTP request
- **Payload generation**: uses `cJSON_PrintUnformatted()`

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
- `GET /scan_wifi`: triggers a live Wi‑Fi scan (when possible) and returns JSON array of discovered APs
- `GET /get_cached_wifi_list`: returns the current cached scan results (if any)

Recommended validations (server-side):

- **Device name**: 1–31 chars, alnum + hyphen, cannot start/end with hyphen
- **Host address**: non-empty (`host_addr` in this repo; may be IPv4 or hostname, optionally `:port`)
- **Wi-Fi SSID**: non-empty

Implementation details in this repo (useful gotchas):

- The provisioning page UI includes a Wi‑Fi dropdown populated from the cached scan list and a **“Scan Wi‑Fi”** button that calls `GET /scan_wifi` via JavaScript.
- The server sets `Cache-Control: no-store` and `Pragma: no-cache` on the provisioning page to avoid stale HTML in browsers.
- `POST /save` reads the request body into a fixed buffer (256 bytes). If you allow very long `host_addr` or password values, you may need to raise that limit.
- `POST /save` and `POST /update` share the same parser/validator and **reboot the device ~2 seconds** after returning the success HTML.
- `GET /scan_wifi` does a **blocking scan** when possible; if a scan can't start (Wi‑Fi busy), the handler returns the **last cached list** instead of failing the UI.
- Server config: `max_uri_handlers = 8`, `stack_size = 8192`.

### Normal mode

- `GET /`: status page
- `GET /api/status`: JSON with device name and status integer (0/1)
- `GET /edit`: shows the same form as provisioning but pre-filled; submits to `/update`
- `POST /update`: same parsing/validation logic as `/save`, then reboots
- `GET /scan_wifi`: available in normal mode as well (used by `/edit`)

Important repo behavior:

- If `g_app_config.provisioned == false`, `web_server_start()` will **always** register the provisioning handlers (even if `g_app_mode` changes), so a half-provisioned device can’t accidentally show a “normal” UI.

Example response:

```json
{"name":"Kitchen-Sensor","status":0}
```

---

## 11. Main application flow (`main.c`)

High-level boot flow:

0. Hardware constants (this repo):
   - `PIR_SENSOR_GPIO = GPIO4`
   - `PROV_AP_SSID = "new-motion-sensor"`
1. Initialize defaults for `g_app_config` (this repo default device name is `"esp32-sensor"`)
2. `nvs_manager_init()`
3. `led_indicator_init()`
4. `motion_sensor_init(PIR_SENSOR_GPIO, on_motion_event)` (repo implementation)
5. `nvs_manager_load_config(&g_app_config)`
5. Decide:
   - **Provisioning** if config is missing/invalid
   - **Connecting** if config is valid + provisioned
6. `wifi_manager_init(on_wifi_event, hostname_or_NULL)`
7. Start AP or STA
8. LED pattern depends on mode

Wi-Fi callback (`on_wifi_event`) should implement:

- `WIFI_EVENT_AP_START`:
  - start provisioning web server (repo starts the server from this event to avoid a race/double-start)
  - (repo implementation) kick an initial background Wi‑Fi scan so `/` can render a network dropdown immediately
- `WIFI_EVENT_STA_START`:
  - set `MODE_CONNECTING`, fast blink
- `IP_EVENT_STA_GOT_IP`:
  - set `MODE_NORMAL`
  - sync LED to the current sensor state (repo calls `on_motion_event(motion_sensor_get_status())`)
  - (repo behavior) this also triggers a **host POST** once, on connect, to publish current state
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

### Repo implementation: MH‑SR602 motion sensor (GPIO4)

This repo includes `motion_sensor.c` and uses it as the status provider:

- **GPIO**: `GPIO4` (`PIR_SENSOR_GPIO` in `main.c`)
- **Interrupts**: `GPIO_INTR_ANYEDGE` with an ISR that pushes the GPIO number into a FreeRTOS queue
- **Task**: `motion_sensor_task` reads pin level on queue events and updates `motion_status_t` (`MOTION_DETECTED` / `MOTION_NONE`)
- **Callbacks**: the module invokes a callback **only on transitions** (no “initial state” callback)
- **Initial state**: the status is still tracked immediately; the repo syncs the LED (and sends one host update) on Wi‑Fi connect by calling `motion_sensor_get_status()`
- **No software debounce/cooldown**: state changes are forwarded immediately; if you need rate limiting, add it where you trigger `host_client_send_motion_update(...)`

---

## 13. HTML approaches (repo: `snprintf` format strings; template: token replacement)

This repo embeds HTML as C string literals and renders them with `snprintf()` (see `web_server.c`):

- Provisioning/edit form: `PROV_HTML_FORM` (uses `snprintf(..., "%.*s", ...)`)
- Normal status page: `NORMAL_HTML_PAGE`

If you embed HTML in a `snprintf` format string, literal `%` must be written as `%%` (e.g., `width: 100%%;`). You can also choose a **token replacement** approach (below) if you prefer not to use `snprintf` formatting.

### Token replacement samples (optional)

These HTML samples use **`%%...%%` tokens** that you can replace at runtime (server-side).

> The `%%PLACEHOLDER%%` tokens below are shown as **raw HTML tokens** (not `snprintf` format strings).

### Provisioning page (`GET /`)

Required tokens:

- `%%CURRENT_DEVICE_NAME%%`
- `%%CURRENT_HOST_ADDR%%`

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

      <label for="host_addr">Host Server Address (IP or Hostname:Port):</label>
      <input type="text" id="host_addr" name="host_addr" value="%%CURRENT_HOST_ADDR%%" required>

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
- Example `sdkconfig.defaults.*` values that enable blink-only settings

Keeping them won’t necessarily break the build, but it **makes the project harder to understand** and can confuse future users of the template.
