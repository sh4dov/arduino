# Project: ESP32-C3 LED Controller Server

This project is an ESP32-C3 based server designed to control multiple LED strips. It acts as a hub, receiving triggers from remote motion sensor devices and activating corresponding LED strips with configurable brightness and timing. It also hosts a web interface for manual control and configuration, and provides an API for integration with a central server.

## 1. Project Overview

The device operates as a smart lighting controller. Its core purpose is to:
- Receive motion detection events from remote sensor boards via an HTTP endpoint.
- Control multiple single-color LED strips via GPIOs using PWM for brightness.
- Implement smooth fade-in/fade-out effects for all LED state changes.
- Provide a web-based user interface for manual control and advanced settings configuration.
- Group multiple LED strips into controllable "Items" (e.g., "Kitchen Countertop").
- Persist all configuration (Wi-Fi, device settings, brightness levels) across reboots.
- Be discoverable on the local network via mDNS (e.g., `http://led-server.local`).

### Intended Architecture & Modules

The project will follow a modular architecture. The main components to be developed in the `main/` directory are:

- **`main.c`**: The main application entry point. It will initialize all modules and manage the application's primary state (Provisioning, Normal, Wi-Fi Disconnected).
- **`app_config.c/.h`**: Handles persistent storage of the device configuration (Wi-Fi credentials, server name, Item brightness, timers) in Non-Volatile Storage (NVS).
- **`wifi_manager.c/.h`**: Manages all Wi-Fi operations, including starting the soft Access Point (AP) in Provisioning Mode and connecting to the configured network in Normal Mode.
- **`web_server.c/.h`**: Implements the web user interface and API. This includes the main control page, the advanced settings page, and the API endpoints for sensor input (`/api/motionSensor`) and central server control (`/api/status`, `/api/control`).
- **`led_control.c/.h`**: Manages the LED strips using the `LEDC` peripheral. This module will handle setting GPIOs, generating PWM signals for brightness, and executing fade-in/fade-out animations.
- **`automation.c/.h`**: Contains the core control logic. It manages sensor-to-item mapping, handles the automation and manual override timers, and controls the state transitions between automatic and manual modes for each Item.
- **`mdns_service.c/.h`**: Implements mDNS discovery, making the device accessible via a `.local` hostname.
- **`status_led.c/.h`**: Provides visual feedback on the device's status using the onboard LED on `GPIO 8`, according to the states defined in the PRD.

### Technologies
- **Framework**: ESP-IDF v5.x
- **Hardware**: ESP32-C3
- **Primary Language**: C
- **Build System**: CMake

## 2. Building and Running

**Important Build Instructions:**

This project should be built, flashed, and monitored using an integrated development environment (IDE), such as the **Visual Studio Code ESP-IDF Extension**. The IDE provides a reliable and integrated workflow.

**Do not run `idf.py` commands (like `idf.py flash monitor`) directly from the terminal.** Instead, please ask me to initiate the build process for you.

### One-Time Setup

The only command you may need to run directly is for initial project setup:

1.  **Set Target Chip:**
    (This only needs to be done once per project)
    ```sh
    idf.py set-target esp32c3
    ```
