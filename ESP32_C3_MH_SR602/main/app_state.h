#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h> // For bool type in app_config_t
#include "nvs_manager.h" // For app_config_t definition

// Application operational modes
typedef enum {
    MODE_PROVISIONING,
    MODE_CONNECTING,
    MODE_NORMAL,
} app_mode_t;

// Global application state variables
extern app_mode_t g_app_mode;
extern app_config_t g_app_config;

// LED indicator patterns
typedef enum {
    LED_SOLID_OFF,
    LED_SOLID_ON,
    LED_SLOW_BLINK,
    LED_FAST_BLINK,
} led_pattern_t;

// Motion sensor status
typedef enum {
    MOTION_DETECTED,
    MOTION_NONE,
} motion_status_t;

#endif // APP_STATE_H
