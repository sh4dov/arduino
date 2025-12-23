#include "app_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "app_config";
#define NVS_NAMESPACE "led_server"
#define CONFIG_KEY "app_config"

static app_config_t g_app_config;

// Hard-coded definition of all controllable items
// -1 for led_gpios marks the end of the list for an item.
// NULL for sensor_names marks the end of the list for an item.
// An item with a NULL name marks the end of the item list.
static const item_static_config_t g_item_static_config[MAX_ITEMS] = {
    {
        .name = "Kitchen Counter",
        .led_gpios = {2, 3}, // Using GPIOs 2 and 3
        .sensor_names = {"kitchen-sensor-1", NULL}
    },
    {
        .name = "Stairs",
        .led_gpios = {4, -1}, // Using GPIO 4
        .sensor_names = {"stairs-sensor-1", "stairs-sensor-2"}
    },
    // End of list marker
    {.name = NULL}
};

static int g_num_items = -1;

static void app_config_load_defaults(void)
{
    memset(&g_app_config, 0, sizeof(app_config_t));
    g_app_config.provisioned = false;
    strncpy(g_app_config.server_name, "led-server", sizeof(g_app_config.server_name) - 1);
    g_app_config.automation_timer_sec = 30;
    g_app_config.manual_override_timer_hr = 1;
    g_app_config.fade_duration_sec = 1;
    for (int i = 0; i < MAX_ITEMS; i++) {
        g_app_config.item_brightness[i] = 50;
    }
    ESP_LOGI(TAG, "Loaded default configuration");
}

esp_err_t app_config_load(void)
{
    if (g_num_items == -1) {
        g_num_items = 0;
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (g_item_static_config[i].name != NULL) {
                g_num_items++;
            } else {
                break; // Stop at the first NULL name
            }
        }
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        app_config_load_defaults();
        return err;
    }

    size_t required_size = sizeof(app_config_t);
    err = nvs_get_blob(nvs_handle, CONFIG_KEY, &g_app_config, &required_size);

    if (err == ESP_OK && required_size == sizeof(app_config_t) && g_app_config.provisioned) {
        ESP_LOGI(TAG, "Successfully loaded configuration from NVS");
    } else {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "Configuration not found in NVS. Loading defaults.");
        } else if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error reading configuration from NVS: %s. Loading defaults.", esp_err_to_name(err));
        } else if (required_size != sizeof(app_config_t)) {
            ESP_LOGW(TAG, "NVS data size mismatch. Expected %d, got %d. Loading defaults.", sizeof(app_config_t), (int)required_size);
        } else { // !g_app_config.provisioned
            ESP_LOGI(TAG, "Device is not provisioned. Loading defaults.");
        }
        app_config_load_defaults();
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t app_config_save(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(nvs_handle, CONFIG_KEY, &g_app_config, sizeof(app_config_t));
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Successfully saved configuration to NVS");
        } else {
            ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "NVS set blob failed: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

app_config_t* app_config_get(void)
{
    return &g_app_config;
}

const item_static_config_t* app_config_get_items(void) {
    return g_item_static_config;
}

int app_config_get_num_items(void) {
    if (g_num_items == -1) {
        // This should have been called by app_config_load(), but as a fallback:
        g_num_items = 0;
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (g_item_static_config[i].name != NULL) {
                g_num_items++;
            } else {
                break;
            }
        }
         ESP_LOGW(TAG, "g_num_items was not initialized. Call app_config_load first.");
    }
    return g_num_items;
}

int app_config_get_item_index_by_name(const char* name) {
    if (name == NULL) return -1;
    int num_items = app_config_get_num_items();
    for (int i = 0; i < num_items; i++) {
        if (strcmp(g_item_static_config[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}
