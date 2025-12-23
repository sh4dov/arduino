#ifndef AUTOMATION_H
#define AUTOMATION_H

#include <stdint.h>
#include <stdbool.h>

// Enum for item control mode
typedef enum {
    MODE_AUTOMATIC,
    MODE_MANUAL
} item_control_mode_t;

// Struct to hold the dynamic state for each item
typedef struct {
    item_control_mode_t control_mode;
    bool is_on;
    uint8_t brightness; // Current brightness (0-100)
    int64_t automation_timer_end_time; // Time when automation timer expires (us)
} item_state_t;

/**
 * @brief Initializes the automation module, creates the automation task.
 */
void automation_init(void);

/**
 * @brief Handles a motion sensor event.
 * 
 * @param sensor_name The name of the sensor that triggered the event.
 * @param state The state of the motion sensor (1 for motion detected, 0 for motion ended).
 */
void automation_handle_motion_event(const char* sensor_name, uint8_t state);

/**
 * @brief Sets an item to manual control mode.
 * 
 * @param item_index The index of the item to control.
 * @param on True to turn the item on, false to turn it off.
 * @param brightness The target brightness (0-100).
 */
void automation_set_item_manual(int item_index, bool on, uint8_t brightness);

/**
 * @brief Gets the current dynamic state of an item.
 * 
 * @param item_index The index of the item.
 * @param state Pointer to an item_state_t struct to fill with the current state.
 * @return True if the item_index is valid, false otherwise.
 */
bool automation_get_item_state(int item_index, item_state_t* state);

#endif // AUTOMATION_H
