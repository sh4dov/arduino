# ESP32-C3 Motion Sensor: Implementation Plan

## 1. Overview

This document outlines the technical implementation strategy for the ESP32-C3 Motion Sensor IoT Device, as specified in `PRD.md`. The plan details the proposed architecture, component breakdown, development phases, and testing strategy.

## 2. Core Architecture & Components

The project will be built using the ESP-IDF framework, leveraging its multitasking capabilities and rich set of libraries.

-   **Framework:** ESP-IDF (latest stable version)
-   **Language:** C/C++
-   **RTOS:** FreeRTOS will be used to manage distinct tasks for Wi-Fi, sensor monitoring, and HTTP communication, ensuring a responsive, non-blocking system.
-   **Key ESP-IDF Components:**
    -   `nvs_flash`: For persistent storage of configuration.
    -   `esp_wifi`: For managing both Access Point (AP) and Station (STA) modes.
    -   `esp_netif`: For network interface management.
    -   `esp_http_server`: To provide the web-based configuration portal and the status API/UI.
    -   `esp-tls`, `esp_http_client`: To send POST requests to the user-defined host server.
    -   `mdns`: To broadcast the device on the local network via `http://<Device-Name>.local/`.

## 3. Hardware Configuration

-   **PIR Sensor (MH-SR602):** The sensor's digital `OUT` pin will be connected to **GPIO4**. This pin will be configured as a digital input with an internal pull-down resistor.
-   **Onboard LED:** The ESP32-C3's common built-in LED on **GPIO8** will be used. It will be controlled via the RMT peripheral for color and brightness, but for this project's simple on/off/blink states, basic GPIO control is sufficient.

## 4. Project Structure

The codebase will be organized into logical modules to promote separation of concerns and maintainability. All new code will reside in the `main/` directory.

-   `main.c`: The application entry point (`app_main`). Responsible for initializing components and creating the primary FreeRTOS tasks.
-   `app_state.h`: A central header defining shared data structures and enumerations for the device's operational mode (`PROVISIONING`, `CONNECTING`, `NORMAL`) and motion status.
-   **Modules (as `.c`/`.h` pairs):**
    -   `nvs_manager`: Handles saving and loading the device configuration `struct` to/from Non-Volatile Storage.
    -   `wifi_manager`: Manages all Wi-Fi operations, including starting the AP for provisioning, and connecting as a station in normal mode (with reconnection logic).
    -   `led_indicator`: Provides functions to control the onboard LED based on the current application state (e.g., `led_set_pattern(FAST_BLINK)`).
    -   `motion_sensor`: A dedicated task will monitor the sensor's GPIO pin, manage the motion state, and handle the 1-second software cooldown period.
    -   `web_server`: Starts and manages the `esp_http_server`. It will register different URI handlers depending on the device's operational mode.
    -   `host_client`: Contains the logic for sending motion state updates via HTTP POST to the configured host server, including the 5-second retry mechanism.

## 5. Implementation Phases

### Phase 1: Core System & Hardware Abstraction

1.  **Task:** Initialize base project, `sdkconfig`, and GPIO.
    -   **Details:** Set up `app_main`. Configure GPIO for the sensor and LED.
2.  **Task:** Create `led_indicator` module.
    -   **Details:** Implement functions for `SLOW_BLINK`, `FAST_BLINK`, `SOLID_ON`, and `OFF` patterns using FreeRTOS tasks and delays.
3.  **Task:** Create `motion_sensor` module.
    -   **Details:** Set up a GPIO interrupt to detect rising and falling edges from the sensor. Implement the 1-second cooldown logic after a `HIGH`->`LOW` transition.

### Phase 2: Configuration & Persistence

1.  **Task:** Implement `nvs_manager` module.
    -   **Details:** Define a `struct` to hold all configuration data. Write `save_config_to_nvs()` and `load_config_from_nvs()` functions.
2.  **Task:** Implement boot logic in `main.c`.
    -   **Details:** On startup, call `load_config_from_nvs()`. If it returns success, proceed to Phase 4. Otherwise, enter Provisioning Mode (Phase 3).

### Phase 3: Provisioning Mode (AP)

1.  **Task:** Extend `wifi_manager` for AP mode.
    -   **Details:** Create `wifi_init_softap()` to configure and start an open Wi-Fi network with the SSID `new-motion-sensor`.
2.  **Task:** Implement the provisioning web server.
    -   **Details:** In `web_server.c`, create a handler for the root `/` that serves a hardcoded HTML form. Create a handler for `/save` (POST) that parses the form data, performs standard hostname validation on the device name, calls `nvs_manager` to save the configuration, serves a success page, and calls `esp_restart()` to reboot the device.

### Phase 4: Normal Mode (Station)

1.  **Task:** Extend `wifi_manager` for STA mode.
    -   **Details:** Create `wifi_init_sta()` which uses the loaded configuration to connect to the user's Wi-Fi. Implement an event handler to listen for connection/disconnection events and manage the `CONNECTING` state and reconnection logic. The LED pattern will be set to `FAST_BLINK` during these attempts.
2.  **Task:** Implement mDNS service.
    -   **Details:** After a successful Wi-Fi connection, initialize the mDNS service to advertise `_http` on port 80, using the device name from configuration.
3.  **Task:** Implement the status web interface and API.
    -   **Details:** In `web_server.c`, register handlers for:
        -   `/`: Serves a simple HTML page displaying the device name and current motion status.
        -   `/api/status`: Responds with a JSON object: `{"name": "...", "status": 0/1}`.

### Phase 5: Host Communication & Integration

1.  **Task:** Implement `host_client` module.
    -   **Details:** Create a function `send_motion_update(bool state)` that sends a POST request. This function will be non-blocking and will manage the retry logic (up to 3 times with a 5s delay between failures) in a separate FreeRTOS task.
2.  **Task:** Integrate all modules.
    -   **Details:** Use a FreeRTOS queue as the central event bus. The `motion_sensor` task will post state-change events to the queue. The main application task will process these events, update the LED, and trigger the `host_client` to send the update. This ensures a decoupled architecture.

## 6. Testing & Validation Strategy

1.  **Initial Setup:** Verify the device boots into AP mode when NVS is empty.
2.  **Provisioning:** Connect to the AP, submit the configuration form, and confirm the device reboots.
3.  **Wi-Fi Connection:** Verify the device connects to the configured Wi-Fi and the LED indicates the correct state (fast blink while connecting, off when idle).
4.  **Network Services:**
    -   Access the device via `http://<Device-Name>.local/`.
    -   Verify the status page and the `/api/status` endpoint show correct information.
5.  **Motion Detection:**
    -   Trigger motion and verify the LED turns on and a POST request is sent to the host.
    -   Confirm the cooldown prevents rapid-fire requests.
    -   Verify that when motion stops, the LED turns off and a second POST request is sent.
6.  **Error Handling:**
    -   Temporarily turn off the Wi-Fi AP to test the device's reconnection logic and LED pattern.
    -   Configure an invalid host IP to test the POST retry mechanism.
