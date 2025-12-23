#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdbool.h>

// Enum for the state of the status LED
typedef enum {
    STATUS_LED_PROVISIONING,      // Fast blink
    STATUS_LED_WIFI_CONNECTING,   // Slow blink (1s interval)
    STATUS_LED_NORMAL,            // Solid off
} status_led_state_t;

/**
 * @brief Initializes the status LED GPIO and timers.
 */
void status_led_init(void);

/**
 * @brief Sets the operational state of the status LED.
 * 
 * @param state The state to set.
 */
void status_led_set_state(status_led_state_t state);

#endif // STATUS_LED_H
