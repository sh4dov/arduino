#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define MAX_ITEMS 6
#define MAX_LED_STRIPS_PER_ITEM 2
#define MAX_SENSORS_PER_ITEM 2

typedef struct {
    char server_name[32];
    char wifi_ssid[32];
    char wifi_password[64];
    bool provisioned;

    uint32_t automation_timer_sec;
    uint32_t manual_override_timer_hr;
    uint32_t fade_duration_sec;
    uint8_t item_brightness[MAX_ITEMS];
} app_config_t;

typedef struct {
    const char* name;
    const int led_gpios[MAX_LED_STRIPS_PER_ITEM];
    const char* sensor_names[MAX_SENSORS_PER_ITEM];
} item_static_config_t;

esp_err_t app_config_load(void);
esp_err_t app_config_save(void);
app_config_t* app_config_get(void);
const item_static_config_t* app_config_get_items(void);
int app_config_get_num_items(void);
int app_config_get_item_index_by_name(const char* name);

#endif // APP_CONFIG_H
