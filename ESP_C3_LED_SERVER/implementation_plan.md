# ESP32-C3 LED Controller Server: Implementation Plan

## 1. Overview

This document outlines the implementation strategy for the ESP32-C3 LED Controller Server. The plan is to adapt the architecture from the `ESP32_C3_IOT_TEMPLATE` and extend it with new modules to meet the specific requirements defined in the `PRD.md`.

The implementation will be divided into logical phases to ensure a structured and testable development process.

## 2. Project Structure

The project will be organized into the following modules within the `main/` directory:

-   `main.c`: The main application entry point, responsible for initializing modules and managing the primary application state machine (Provisioning, Connecting, Normal, Wi-Fi Disconnected).
-   `app_config.c`/`.h`: **(Modified from template's `nvs_manager`)** Manages NVS persistence for all configuration data. It will also define the hard-coded "Item" configurations and default values.
-   `led_control.c`/`.h`: **(New Module)** Manages the LED strips using the `LEDC` peripheral. This module will handle setting GPIOs, generating PWM signals for brightness, and executing non-blocking fade-in/fade-out animations.
-   `automation.c`/`.h`: **(New Module)** Contains the core control logic. It manages sensor-to-item mapping, handles the Automation and Manual Override timers, and controls the state transitions between Automatic and Manual modes for each Item.
-   `web_server.c`/`.h`: **(Modified from template)** Implements the web UI and API. This includes the main control page (`/`), the advanced settings page (`/settings`), and the API endpoints (`/api/motionSensor`, `/api/status`, `/api/control`).
-   `wifi_manager.c`/`.h`: **(From template)** Manages all Wi-Fi operations, including the provisioning AP and STA connection.
-   `mdns_service.c`/`.h`: **(From template)** Implements mDNS discovery.
-   `status_led.c`/`.h`: **(From template)** Provides visual feedback on the device's status using the onboard LED on `GPIO 8`.

## 3. Data Structures & Configuration

### 3.1. Item Configuration (`app_config.c`)

Items will be hard-coded in an array. This provides a simple and robust initial setup. A maximum of 6 Items and 6 total LED strips will be assumed.

**Sample Item Configuration:**

```c
// In app_config.c
#define MAX_LED_STRIPS_PER_ITEM 2
#define MAX_SENSORS_PER_ITEM 2

typedef struct {
    const char* name;
    const int led_gpios[MAX_LED_STRIPS_PER_ITEM];
    const char* sensor_names[MAX_SENSORS_PER_ITEM];
} item_static_config_t;

// Hard-coded definition of all controllable items
static const item_static_config_t g_item_static_config[MAX_ITEMS] = {
    {
        .name = "Kitchen Counter",
        .led_gpios = {2, 3},
        .sensor_names = {"kitchen-sensor-1"}
    },
    {
        .name = "Stairs",
        .led_gpios = {4},
        .sensor_names = {"stairs-sensor-1", "stairs-sensor-2"}
    }
    // ... up to 6 items
};
```

### 3.2. Dynamic State (`automation.c`)

A corresponding array will hold the dynamic state for each item.

```c
typedef enum {
    MODE_AUTOMATIC,
    MODE_MANUAL
} item_control_mode_t;

typedef struct {
    item_control_mode_t control_mode;
    bool is_on;
    uint8_t brightness; // Current brightness (0-100)
    int64_t automation_timer_end_time;
} item_state_t;

static item_state_t g_item_states[MAX_ITEMS];
```

### 3.3. Persistent Configuration (`app_config.h`)

The main configuration struct, stored in NVS.

```c
typedef struct {
    char server_name[32];
    char wifi_ssid[32];
    char wifi_password[64];
    bool provisioned;

    uint32_t automation_timer_sec; // Default: 30s
    uint32_t manual_override_timer_hr; // Default: 1h
    uint32_t fade_duration_sec; // Default: 1s
    uint8_t item_brightness[MAX_ITEMS]; // Default: 50% for all
} app_config_t;
```

## 4. Module Implementation Details

### `app_config.c`
-   Extend `nvs_manager_load_config` and `save_config` from the template to handle the new `app_config_t` fields, including iterating through `item_brightness`.
-   On first boot, populate `app_config_t` with the specified default values (Automation: 30s, Manual Override: 1h, Fade: 1s, Brightness: 50%).

### `led_control.c` (New)
-   **`led_control_init()`**: Initializes the `LEDC` timer and all channels required for the configured LED strips (up to 6).
-   **`led_control_fade_to(item_index, target_brightness_percent, duration_ms)`**: Uses the ESP-IDF `ledc_set_fade_with_time` function to start a non-blocking fade for all LEDs in the specified item.
-   **`led_control_set_brightness(item_index, brightness_percent)`**: Immediately sets the brightness for all LEDs in an item.
-   **`led_control_handle_boot_sequence(step)`**: A function to manage the boot-up LED effect (blinking, then solid).

### `automation.c` (New)

- A FreeRTOS task, `automation_task`, will run every second to check timers.

- **Global Manual Override Timer**: A single `int64_t` variable, `g_manual_override_end_time`, will track the expiration time. 

    - It is updated to `esp_timer_get_time() + (manual_override_timer_hr * ...)` whenever an item is manually turned **ON**.

    - When `automation_task` detects this timer has expired, it iterates through all items, reverts those in `MODE_MANUAL` to `MODE_AUTOMATIC`, sets `is_on` to false, and triggers a fade-out.

- **Manual Control Handling**:

    - `Manual ON`: Sets `MODE_MANUAL`, resets the global override timer, and fades in.

    - `Manual OFF`: Sets `MODE_AUTOMATIC`, sets `is_on = false`, cancels any active automation timer for that item, and fades out.

- **Per-Item Automation Timer**: When a motion trigger (`state: 1`) is received for an item in `MODE_AUTOMATIC`, its `automation_timer_end_time` is updated and it fades in. The `automation_task` will fade out items whose automation timers have expired.

- Fades will be interruptible. If new motion arrives while fading out, the logic will call `led_control_fade_to` to fade back in.

### `web_server.c`
-   **`/` (Main Page)**: Will dynamically render a list of items from the configuration, each with a toggle switch reflecting its `is_on` state.
-   **`/settings` (Advanced Settings)**: A form with inputs for the global timers and sliders for each item's configured brightness.
-   **`POST /settings`**: Handler will parse all form fields, save them to the `app_config_t` struct in NVS, and trigger a reboot.
-   **`POST /api/motionSensor`**: Will parse the sensor name, find the corresponding item, and call the `automation` module to handle the event. It will ignore `state: 0`.
-   **`POST /api/control`**: Will parse the item name, state, and optional brightness. It will then call the `automation` module to set the item to manual mode and apply the state. API-driven brightness changes are temporary and not saved to NVS.
-   **`GET /api/status`**: Will construct a JSON response by iterating through the item configurations and their current states.

### `main.c`
-   The main state machine will be updated to orchestrate the new modules.
-   **Boot Sequence**: Upon power-on, `main` will initiate the LED boot sequence. On Wi-Fi connection, it will signal `led_control` to proceed with the "solid-on for 10s then fade-out" part of the sequence.
-   **Wi-Fi Disconnected**: Upon receiving the disconnect event, `main` will call a function in `led_control` to set all strips to a fixed 10% brightness.

## 5. Phased Development Plan

1.  **Phase 1: Foundation & Core Services**
    -   [ ] Set up the project structure, copying `wifi_manager`, `mdns_service`, and `status_led` from the template.
    -   [ ] Implement the extended `app_config` module with the new data structures and NVS logic.
    -   [ ] Create the hard-coded Item configuration.
    -   [ ] Verify that provisioning, Wi-Fi connection, and mDNS discovery work.

2.  **Phase 2: LED Control & Boot Sequence**
    -   [ ] Create the `led_control` module to initialize LEDC channels.
    -   [ ] Implement basic functions to turn LEDs on/off to a specific brightness.
    -   [ ] Implement the special boot-up LED sequence in coordination with `main.c`.

3.  **Phase 3: Automation Logic**
    -   [ ] Create the `automation` module with its state-tracking structures and periodic task.
    -   [ ] Implement the `POST /api/motionSensor` endpoint.
    -   [ ] Implement the core automation logic: turning items on with motion and deactivating them when the Automation Timer expires.

4.  **Phase 4: Fading and Manual Control**
    -   [ ] Implement hardware-accelerated fading in `led_control`.
    -   [ ] Integrate fading into the `automation` module.
    -   [ ] Implement the Manual Override logic and timer in the `automation` module.
    -   [ ] Implement the `/` main page UI with on/off toggles to trigger manual control.

5.  **Phase 5: Settings, API & Finalization**
    -   [ ] Implement the `/settings` web page and its `POST` handler to save configuration to NVS.
    -   [ ] Implement the `GET /api/status` and `POST /api/control` endpoints.
    -   [ ] Conduct end-to-end testing of all features.
    -   [ ] Code cleanup and final review.
