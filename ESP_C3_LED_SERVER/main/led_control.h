#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

// Enum for boot sequence steps
typedef enum {
    BOOT_SEQ_INITIAL_BLINK,      // Blinking LEDs during initial boot
    BOOT_SEQ_START_WIFI_CONNECT, // Blinking LEDs during Wi-Fi connection attempt
    BOOT_SEQ_WIFI_CONNECTED,     // Solid on for 10 seconds after connection
    BOOT_SEQ_NORMAL_OPERATION,   // Fade out and enter normal operation
} led_boot_sequence_step_t;

/**
 * @brief Initializes the LEDC timer and channels for all configured LED strips.
 */
void led_control_init(void);

/**
 * @brief Sets the brightness for all LEDs in a specific item.
 *
 * @param item_index The index of the item.
 * @param brightness_percent The target brightness (0-100).
 */
void led_control_set_brightness(int item_index, uint8_t brightness_percent);

/**
 * @brief Fades the brightness for all LEDs in a specific item.
 *
 * @param item_index The index of the item.
 * @param target_brightness_percent The target brightness (0-100).
 * @param duration_ms The duration of the fade in milliseconds.
 */
void led_control_fade_to(int item_index, uint8_t target_brightness_percent, uint32_t duration_ms);

/**
 * @brief Handles the different steps of the LED boot sequence.
 *
 * @param step The boot sequence step to execute.
 */
void led_control_handle_boot_sequence(led_boot_sequence_step_t step);

#endif // LED_CONTROL_H
