# Project: ESP32-C3 Motion Sensor IoT Device

This is a complete IoT device project for an ESP32-C3 microcontroller using the ESP-IDF framework. It features a motion sensor, Wi-Fi connectivity, a web-based user interface, and remote host notification capabilities.

## 1. Project Overview

The device operates as a smart motion sensor. Its core purpose is to:
- Detect motion using an MH-SR602 PIR sensor.
- Connect to a local Wi-Fi network.
- Notify a remote host server via HTTP POST when motion is detected.
- Provide a local web interface for status monitoring and configuration.
- Be discoverable on the local network via mDNS (e.g., `http://my-sensor.local`).

### Architecture & Modules

The project follows a modular architecture, with clear separation of concerns. The main components are located in the `main/` directory:

- **`main.c`**: The main application entry point. It initializes all modules and manages the application's state (provisioning, connecting, normal).
- **`app_state.h`**: Defines shared data types and application modes (`app_mode_t`, `motion_status_t`).
- **`nvs_manager.c/.h`**: Handles persistent storage of the device configuration (Wi-Fi credentials, device name, host address) in Non-Volatile Storage (NVS).
- **`wifi_manager.c/.h`**: Manages all Wi-Fi operations, including:
    - **Provisioning Mode**: Starts a soft Access Point (AP) named `new-motion-sensor`.
    - **Station (STA) Mode**: Connects to the configured Wi-Fi network.
    - Wi-Fi scanning.
- **`web_server.c/.h`**: Implements the web user interface and API.
    - In provisioning mode, it serves a form to configure the device.
    - In normal mode, it serves a status page, an `/edit` page, and a JSON API endpoint (`/api/status`).
- **`motion_sensor.c/.h`**: Interfaces with the MH-SR602 PIR motion sensor on `GPIO4` using interrupts.
- **`host_client.c/.h`**: Sends motion status updates to a configurable remote host via HTTP POST requests to `/api/motionSensor`.
- **`mdns_service.c/.h`**: Implements mDNS discovery, making the device accessible via a `.local` hostname.
- **`led_indicator.c/.h`**: Provides visual feedback on the device's status using an LED on `GPIO8` (active-low).
    - Slow Blink: Provisioning mode.
    - Fast Blink: Connecting to Wi-Fi.
    - Solid Off: Normal mode, no motion.
    - Solid On: Normal mode, motion detected.

### Technologies
- **Framework**: ESP-IDF v5.x
- **Hardware**: ESP32-C3, MH-SR602 PIR Sensor
- **Primary Language**: C
- **Build System**: CMake

## 2. Building and Running

**Important Note:** It is recommended to compile this project using an IDE (such as Visual Studio Code with the ESP-IDF Extension) rather than running `idf.py` commands directly. The IDE provides an integrated development experience including build, flash, and monitoring capabilities.

The project is built and managed using the ESP-IDF toolchain.

### Prerequisites
- ESP-IDF environment must be installed and sourced.
- A connected ESP32-C3 development board.

### Key Commands

1.  **Set Target Chip:**
    (This only needs to be done once per project)
    ```sh
    idf.py set-target esp32c3
    ```

2.  **Configure Project (Optional):**
    Open the configuration menu. Most settings are handled by the web UI, but this can be used for advanced IDF settings.
    ```sh
    idf.py menuconfig
    ```

3.  **Build, Flash, and Monitor:**
    This command compiles the project, flashes the binary to the device, and opens a serial monitor to view logs. Replace `<YOUR_SERIAL_PORT>` with your device's port (e.g., `COM3` on Windows, `/dev/ttyUSB0` on Linux).
    ```sh
    idf.py -p <YOUR_SERIAL_PORT> flash monitor
    ```
    To exit the monitor, use the shortcut `Ctrl+]`.

## 3. Development Conventions

- **Modular Design**: Code is organized into modules with specific responsibilities, each with a corresponding `.h` and `.c` file.
- **State Management**: The application's main state is managed in `main.c` via the `g_app_mode` variable, which is updated based on events from the `wifi_manager`.
- **Event-Driven**: The application logic relies on callbacks that are triggered by Wi-Fi events (`on_wifi_event`) and motion sensor interrupts (`on_motion_event`).
- **Configuration**: All user-configurable parameters are stored as a single struct (`app_config_t`) in NVS, managed by `nvs_manager`.
- **Error Handling**: Functions generally return `esp_err_t` to indicate success or failure. `ESP_ERROR_CHECK` is used for critical initializations.
- **Logging**: The ESP-IDF logging library (`esp_log.h`) is used with `TAG`s defined for each module to provide clear and filterable debugging output.
- **HTML in C**: HTML content for the web server is embedded directly as C string literals within `web_server.c`.
