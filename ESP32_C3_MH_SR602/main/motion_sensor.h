#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H

#include "app_state.h"
#include "driver/gpio.h" // For gpio_num_t

// Define a callback function type for motion state changes
typedef void (*motion_detection_callback_t)(motion_status_t status);

// Initialize the motion sensor module
// pir_gpio: The GPIO pin connected to the PIR sensor's OUT pin.
// callback: A function to be called when motion status changes.
void motion_sensor_init(gpio_num_t pir_gpio, motion_detection_callback_t callback);

// Get the current motion sensor status (thread-safe)
motion_status_t motion_sensor_get_status(void);

#endif // MOTION_SENSOR_H
