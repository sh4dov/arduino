#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

// Maximum lengths for configurable strings
#define MAX_DEVICE_NAME_LEN 32
#define MAX_HOST_ADDR_LEN 256 // Accommodate hostnames with optional port
#define MAX_WIFI_SSID_LEN 32
#define MAX_WIFI_PASS_LEN 64

// Structure to hold all configurable device settings
typedef struct {
    char device_name[MAX_DEVICE_NAME_LEN];
    char host_addr[MAX_HOST_ADDR_LEN];
    char wifi_ssid[MAX_WIFI_SSID_LEN];
    char wifi_password[MAX_WIFI_PASS_LEN];
    bool provisioned; // Flag to indicate if initial provisioning has occurred
} app_config_t;

// Initialize NVS flash
esp_err_t nvs_manager_init(void);

// Load configuration from NVS
esp_err_t nvs_manager_load_config(app_config_t *config);

// Save configuration to NVS
esp_err_t nvs_manager_save_config(const app_config_t *config);

// Factory reset (erase all configuration)
esp_err_t nvs_manager_factory_reset(void);

#endif // NVS_MANAGER_H
