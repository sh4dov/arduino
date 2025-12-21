#ifndef APP_STATE_H
#define APP_STATE_H

// Application operational modes
typedef enum {
    MODE_PROVISIONING,
    MODE_CONNECTING,
    MODE_NORMAL,
} app_mode_t;

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
