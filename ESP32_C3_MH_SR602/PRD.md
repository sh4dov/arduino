# Project Requirements Document: ESP32-C3 Motion Sensor IoT Device

## 1. Overview

This document outlines the requirements for an IoT device built using the ESP32-C3 and the MH-SR602 PIR motion sensor. The device will connect to a local Wi-Fi network to report motion changes to a user-defined host server. It will also provide its own simple web interface for status monitoring and a local API to query its current state. The device is designed to be a standalone, single-purpose sensor proxy.

## 2. Core Functionality

-   Detect motion using the MH-SR602 sensor.
-   Provide a one-time web-based configuration portal in Access Point (AP) mode.
-   Connect to a local Wi-Fi network (Station mode) after configuration.
-   Report motion state changes to a configurable host server via HTTP POST.
-   Serve a simple web page displaying the device name and current motion status.
-   Expose a local REST API endpoint to get the current device status and name.
-   Use the onboard LED to provide visual feedback on the device's status.
-   Persist all configuration across power cycles.

## 3. Operational Modes

### 3.1. Provisioning Mode (Access Point)

This mode is for initial device setup.

-   **Activation:** The device enters this mode on first boot when Wi-Fi details are not provided.
-   **Wi-Fi Network:** It creates an open Wi-Fi network with the SSID `new-motion-sensor`.
-   **Configuration Portal:**
    -   Serves a single, open (no password) web page.
    -   The page contains a form with the following fields:
        1.  **Device Name**: A custom name for the device.
        2.  **Host Server IP**: The IP address of the server to send POST requests.
        3.  **Wi-Fi SSID**: The SSID of the local network to connect to.
        4.  **Wi-Fi Password**: The password for the local network.
    -   Upon submission, the form data is saved to persistent storage, and the user is shown a simple success message before the device reboots into Normal Mode.

### 3.2. Normal Mode (Wi-Fi Station)

This is the standard operational mode after a successful configuration.

-   **Connection:** The device connects to the user-specified Wi-Fi network.
-   **Hostname:** It uses the configured **Device Name** as its network hostname.
-   **mDNS:** The device is discoverable on the local network at `http://<Device-Name>.local/`.

## 4. Network Services

### 4.1. Device API (Server)

The device hosts its own local API.

-   **Endpoint:** `GET /api/status`
-   **Response:** A JSON object containing the device's name and current sensor status.
    ```json
    {
      "name": "Living-Room-Sensor",
      "status": 1
    }
    ```
-   **Status Values:**
    -   `1`: Motion is currently detected.
    -   `0`: No motion is detected.

### 4.2. Host Server Communication (Client)

The device sends notifications to the configured host server.

-   **Trigger:** Sent on every state change of the sensor (LOW to HIGH and HIGH to LOW).
-   **Endpoint:** `http://<Host-Server-IP>/api/motionSensor`
-   **Method:** `POST`
-   **Payload:** A JSON object containing the device's name and the new state.
    ```json
    {
      "sensor": "Living-Room-Sensor",
      "state": 1
    }
    ```
-   **State Values:**
    -   `1`: Motion detected.
    -   `0`: No motion.

### 4.3. Device Web Interface

When in Normal Mode, the device serves a basic status page.

-   **Content:** Displays the device's name and its current motion status (e.g., "Motion: Detected" or "Motion: None").
-   **Updates:** The page updates on a manual browser refresh.

## 5. Hardware Interaction

### 5.1. MH-SR602 Sensor

-   The device will read the digital output of the sensor.
-   The sensor's default hardware-defined hold time (approx. 2.5 seconds) will be used. No additional software debouncing or state-holding is required.

### 5.2. Onboard LED

The onboard LED will indicate the device's current state:

| State                       | LED Pattern             |
| --------------------------- | ----------------------- |
| In Configuration (AP) Mode  | Slow Blink (1s on/off)  |
| Connecting to Wi-Fi         | Fast Blink              |
| Connected & Idle (No Motion) | LED Off                 |
| Motion Detected             | Solid LED On            |

## 6. Error Handling & Recovery

-   **Wi-Fi Connection Failure:** On boot, if the device cannot connect to the configured Wi-Fi, it will continuously retry to connect. It will not revert to AP mode.
-   **Host Server POST Failure:** If a POST request to the host server fails, the device will retry the request up to 3 times.

## 7. Persistence

-   All configuration settings (Device Name, Host Server IP, Wi-Fi SSID, Wi-Fi Password) will be stored in the ESP32's Non-Volatile Storage (NVS) to survive reboots and power loss.

## 8. Out of Scope

-   **Security:** The web interface and API are open and intended for a trusted local network. No authentication or encryption will be implemented.
-   **Cloud Integration:** The device will not connect to any external cloud services or MQTT brokers.
-   **Real-time UI:** The status web page does not need to update in real-time.
-   **Advanced Architecture:** The design is for a single-purpose device and does not need to be modular for future sensor expansion.
-   **Hostname for Host Server:** The configuration will only accept an IP address for the host server, not a hostname.

## 9. Build step
- Do not attempt to run idf.py, ask me to build project and wait for my feedback.