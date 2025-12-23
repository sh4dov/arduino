#include "automation.h"
#include "app_config.h"
#include "led_control.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h" // For esp_timer_get_time()

static const char *TAG = "automation";

// Global array to hold the dynamic state for each item
static item_state_t g_item_states[MAX_ITEMS];

// Mutex to protect access to g_item_states and global timers
static SemaphoreHandle_t xAutomationMutex;

// Global manual override end time (us)
static int64_t g_manual_override_end_time = 0;

// Forward declaration of the automation task
static void automation_task(void *pvParameters);

void automation_init(void) {
    xAutomationMutex = xSemaphoreCreateMutex();
    if (xAutomationMutex == NULL) {
        ESP_LOGE(TAG, "Failed to create automation mutex");
        return;
    }

    // Initialize item states
    int num_items = app_config_get_num_items();
    app_config_t* config = app_config_get();
    for (int i = 0; i < num_items; i++) {
        g_item_states[i].control_mode = MODE_AUTOMATIC;
        g_item_states[i].is_on = false;
        g_item_states[i].brightness = config->item_brightness[i]; // Default from config
        g_item_states[i].automation_timer_end_time = 0;
    }
    g_manual_override_end_time = 0;
    ESP_LOGI(TAG, "Automation module initialized. %d items configured.", num_items);

    // Create the automation FreeRTOS task
    xTaskCreate(automation_task, "automation_task", 4096, NULL, 5, NULL);
}

static void automation_task(void *pvParameters) {
    int num_items = app_config_get_num_items();
    app_config_t* config = app_config_get();
    ESP_LOGI(TAG, "Automation task started.");

    while (1) {
        if (xSemaphoreTake(xAutomationMutex, portMAX_DELAY) == pdTRUE) {
            int64_t current_time = esp_timer_get_time(); // Time in microseconds

            // 1. Check Global Manual Override Timer
            if (g_manual_override_end_time != 0 && g_manual_override_end_time < current_time) {
                ESP_LOGI(TAG, "Global manual override expired. Reverting all manual items to AUTOMATIC.");
                for (int i = 0; i < num_items; i++) {
                    if (g_item_states[i].control_mode == MODE_MANUAL) {
                        g_item_states[i].control_mode = MODE_AUTOMATIC;
                        if (g_item_states[i].is_on) {
                            ESP_LOGI(TAG, "Item %s: Fading out after manual override expiration.", app_config_get_items()[i].name);
                            led_control_fade_to(i, 0, config->fade_duration_sec * 1000);
                            g_item_states[i].is_on = false;
                        }
                    }
                }
                g_manual_override_end_time = 0; // Reset global timer
            }

            // 2. Check Per-Item Automation Timers
            for (int i = 0; i < num_items; i++) {
                if (g_item_states[i].control_mode == MODE_AUTOMATIC && g_item_states[i].is_on) {
                    if (g_item_states[i].automation_timer_end_time != 0 && g_item_states[i].automation_timer_end_time < current_time) {
                        ESP_LOGI(TAG, "Item %s: Automation timer expired, fading out.", app_config_get_items()[i].name);
                        led_control_fade_to(i, 0, config->fade_duration_sec * 1000);
                        g_item_states[i].is_on = false;
                        g_item_states[i].automation_timer_end_time = 0; // Reset timer
                    }
                }
            }

            xSemaphoreGive(xAutomationMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Run every second
    }
}

void automation_handle_motion_event(const char* sensor_name, uint8_t state) {
    if (xSemaphoreTake(xAutomationMutex, portMAX_DELAY) == pdTRUE) {
        int num_items = app_config_get_num_items();
        const item_static_config_t* static_items = app_config_get_items();
        app_config_t* config = app_config_get();

        for (int i = 0; i < num_items; i++) {
            for (int j = 0; j < MAX_SENSORS_PER_ITEM; j++) {
                if (static_items[i].sensor_names[j] != NULL && strcmp(static_items[i].sensor_names[j], sensor_name) == 0) {
                    // Motion event for this item
                    if (state == 1) { // Motion detected
                        if (g_item_states[i].control_mode == MODE_AUTOMATIC) {
                            ESP_LOGI(TAG, "Item %s: Motion detected. Fading in.", static_items[i].name);
                            led_control_fade_to(i, config->item_brightness[i], config->fade_duration_sec * 1000);
                            g_item_states[i].is_on = true;
                            g_item_states[i].automation_timer_end_time = esp_timer_get_time() + ((int64_t)config->automation_timer_sec * 1000000ULL);
                        } else {
                            ESP_LOGI(TAG, "Item %s: Motion detected, but in MANUAL mode. Ignoring for automation.", static_items[i].name);
                        }
                    }
                    // For state == 0 (motion ended), we do nothing. The automation timer handles fade out.
                    break; // Sensor found, move to next item
                }
            }
        }
        xSemaphoreGive(xAutomationMutex);
    }
}

void automation_set_item_manual(int item_index, bool on, uint8_t brightness) {
    if (item_index < 0 || item_index >= app_config_get_num_items()) {
        ESP_LOGW(TAG, "Invalid item_index %d for manual control", item_index);
        return;
    }

    if (xSemaphoreTake(xAutomationMutex, portMAX_DELAY) == pdTRUE) {
        app_config_t* config = app_config_get();
        const item_static_config_t* static_items = app_config_get_items();

        if (on) {
            // Manual ON: Set to MANUAL mode and reset GLOBAL timer
            g_item_states[item_index].control_mode = MODE_MANUAL;
            g_item_states[item_index].is_on = true;
            g_item_states[item_index].brightness = brightness;
            
            ESP_LOGI(TAG, "Item %s: Set to MANUAL ON at %d%% brightness. Resetting global override timer.", static_items[item_index].name, brightness);
            led_control_fade_to(item_index, brightness, config->fade_duration_sec * 1000);
            
            // Reset global manual override timer
            g_manual_override_end_time = esp_timer_get_time() + ((int64_t)config->manual_override_timer_hr * 3600ULL * 1000000ULL);
        } else {
            // Manual OFF: Revert to AUTOMATIC mode immediately and turn off
            g_item_states[item_index].control_mode = MODE_AUTOMATIC;
            g_item_states[item_index].is_on = false;
            g_item_states[item_index].automation_timer_end_time = 0; // Clear any pending auto-off
            
            ESP_LOGI(TAG, "Item %s: Set to MANUAL OFF. Reverting to AUTOMATIC mode.", static_items[item_index].name);
            led_control_fade_to(item_index, 0, config->fade_duration_sec * 1000);
        }
        xSemaphoreGive(xAutomationMutex);
    }
}

bool automation_get_item_state(int item_index, item_state_t* state) {
    if (item_index < 0 || item_index >= app_config_get_num_items()) {
        ESP_LOGW(TAG, "Invalid item_index %d for getting state", item_index);
        return false;
    }
    if (state == NULL) {
        return false;
    }

    if (xSemaphoreTake(xAutomationMutex, portMAX_DELAY) == pdTRUE) {
        *state = g_item_states[item_index];
        xSemaphoreGive(xAutomationMutex);
        return true;
    }
    return false;
}