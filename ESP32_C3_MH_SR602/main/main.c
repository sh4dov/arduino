#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "app_state.h"
#include "led_indicator.h"
#include "motion_sensor.h"
#include "nvs_manager.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "mdns_service.h"
#include "host_client.h"

// Hardware Configuration
#define PIR_SENSOR_GPIO 4
#define PROV_AP_SSID "new-motion-sensor"

static const char *TAG = "app_main";
static bool s_initial_scan_kicked = false;

// Global application configuration (will be populated from NVS)
app_config_t g_app_config;
app_mode_t g_app_mode = MODE_PROVISIONING; // Default to provisioning mode

// Callback function for motion sensor events
static void on_motion_event(motion_status_t status) {
    if (g_app_mode != MODE_NORMAL) return;

    if (status == MOTION_DETECTED) {
        ESP_LOGI(TAG, "Main: Motion DETECTED!");
        led_indicator_set_pattern(LED_SOLID_ON);
        host_client_send_motion_update(status);
    } else {
        ESP_LOGI(TAG, "Main: Motion NONE!");
        led_indicator_set_pattern(LED_SOLID_OFF);
        host_client_send_motion_update(status);
    }
}

// Callback function for Wi-Fi events
static void on_wifi_event(app_mode_t mode, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi station started. Connecting...");
        g_app_mode = MODE_CONNECTING;
        led_indicator_set_pattern(LED_FAST_BLINK);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Wi-Fi connected. IP address received.");
        g_app_mode = MODE_NORMAL;
        on_motion_event(motion_sensor_get_status()); // Sync LED with current sensor state
        
        // Start mDNS now that we have an IP
        mdns_service_init(g_app_config.device_name);

        web_server_stop(); // Stop provisioning web server if it was running
        web_server_start(); // Start web server for normal mode
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected. Reconnecting...");
        g_app_mode = MODE_CONNECTING;
        led_indicator_set_pattern(LED_FAST_BLINK);
        mdns_service_stop(); // Stop mDNS since we lost connection
        esp_wifi_connect(); // Try to reconnect
    }
    // Handle AP specific events (e.g., station connected to AP) if necessary
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "Wi-Fi AP started. Starting web server.");
        web_server_start(); // Start web server for provisioning

        // If not provisioned, do an initial Wi-Fi scan so '/' can show networks immediately.
        if (!g_app_config.provisioned && !s_initial_scan_kicked) {
            s_initial_scan_kicked = true;
            esp_err_t scan_err = wifi_manager_scan_networks_async();
            if (scan_err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to start initial Wi-Fi scan: %s", esp_err_to_name(scan_err));
            }
        }
    }
}


void app_main(void)
{
    ESP_LOGI(TAG, "Application startup.");

    // Initialize default configuration
    strncpy(g_app_config.device_name, "esp32-sensor", MAX_DEVICE_NAME_LEN - 1);
    g_app_config.device_name[MAX_DEVICE_NAME_LEN - 1] = '\0';
    strncpy(g_app_config.host_addr, "", MAX_HOST_ADDR_LEN - 1); // Default to empty
    g_app_config.host_addr[MAX_HOST_ADDR_LEN - 1] = '\0';

    // Initialize components
    ESP_ERROR_CHECK(nvs_manager_init());
    led_indicator_init();
    motion_sensor_init(PIR_SENSOR_GPIO, on_motion_event);

    // Load configuration BEFORE initializing Wi-Fi
    esp_err_t err = nvs_manager_load_config(&g_app_config);
    // Always enforce null-termination in case of corrupted NVS or partial writes.
    g_app_config.device_name[MAX_DEVICE_NAME_LEN - 1] = '\0';
    g_app_config.host_addr[MAX_HOST_ADDR_LEN - 1] = '\0';
    g_app_config.wifi_ssid[MAX_WIFI_SSID_LEN - 1] = '\0';
    g_app_config.wifi_password[MAX_WIFI_PASS_LEN - 1] = '\0';

    // Validate that the loaded configuration is complete and valid
    bool config_valid = (err == ESP_OK && 
                        g_app_config.provisioned &&
                        strlen(g_app_config.device_name) > 0 &&
                        strlen(g_app_config.host_addr) > 0 &&
                        strlen(g_app_config.wifi_ssid) > 0);

    if (config_valid) {
        g_app_mode = MODE_CONNECTING;
        ESP_LOGI(TAG, "Device already provisioned. Starting Wi-Fi station.");
        ESP_LOGI(TAG, "Config: Device='%s', Host='%s', SSID='%s'", 
                 g_app_config.device_name, g_app_config.host_addr, g_app_config.wifi_ssid);
        wifi_manager_init(on_wifi_event, g_app_config.device_name);
        wifi_manager_start_sta(&g_app_config);
    } else {
        g_app_mode = MODE_PROVISIONING;
        if (err == ESP_OK && g_app_config.provisioned) {
            ESP_LOGW(TAG, "Stored configuration is incomplete or corrupted. Entering provisioning mode.");
        } else {
            ESP_LOGI(TAG, "Device not yet provisioned. Starting provisioning AP.");
        }
        // The device_name is already set to a default.
        wifi_manager_init(on_wifi_event, NULL); // No hostname for AP mode
        wifi_manager_start_ap(PROV_AP_SSID);
        // Provisioning web server is started from the WIFI_EVENT_AP_START event handler to
        // avoid a race/double-start if the AP comes up before we reach this line.
    }

    // Set initial LED state based on current mode
    switch (g_app_mode) {
        case MODE_PROVISIONING:
            led_indicator_set_pattern(LED_SLOW_BLINK);
            break;
        case MODE_CONNECTING:
            led_indicator_set_pattern(LED_FAST_BLINK);
            break;
        case MODE_NORMAL: // Should not happen here, but for completeness
            led_indicator_set_pattern(LED_SOLID_OFF);
            break;
    }
    
    ESP_LOGI(TAG, "Main task entering idle loop.");
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}
