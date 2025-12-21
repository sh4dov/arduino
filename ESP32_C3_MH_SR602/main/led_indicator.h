#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "app_state.h"

// Initialize the LED indicator module
void led_indicator_init(void);

// Set the desired LED pattern. This function is thread-safe.
void led_indicator_set_pattern(led_pattern_t pattern);

#endif // LED_INDICATOR_H
