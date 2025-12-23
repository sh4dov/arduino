#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#include "app_config.h"
#include "wifi_manager.h"
#include "status_led.h"
#include "web_server.h"
#include "mdns_service.h"
#include "led_control.h"
#include "automation.h"

static const char *TAG = "app_main";

static app_config_t* g_app_config = NULL;
static app_mode_t g_current_app_mode = MODE_PROVISIONING; // Initialize to provisioning

static void on_wifi_event(app_mode_t mode, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Wi-Fi Event: mode=%d, event_base=%s, event_id=%ld", mode, event_base, event_id);
    g_current_app_mode = mode; // Update global app mode

    if (mode == MODE_PROVISIONING) {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
            ESP_LOGI(TAG, "Provisioning AP started. Starting web server.");
            web_server_start();
            status_led_set_state(STATUS_LED_PROVISIONING);
            wifi_manager_scan_networks_async(); // Start initial scan for provisioning UI
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP) {
            ESP_LOGI(TAG, "Provisioning AP stopped. Stopping web server.");
            web_server_stop();
        } else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
            ESP_LOGI(TAG, "Station connected to AP in Provisioning mode.");
            // No action needed for status LED, stays in provisioning state
        }
    } else { // MODE_CONNECTING or MODE_NORMAL
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "STA started. Attempting to connect to AP.");
            status_led_set_state(STATUS_LED_WIFI_CONNECTING);
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "STA disconnected. Retrying connection...");
            status_led_set_state(STATUS_LED_WIFI_CONNECTING);
            mdns_service_stop();
            web_server_stop(); // Stop normal web server if running

            // Per PRD, set all LEDs to 10% brightness on Wi-Fi disconnected
            int num_items = app_config_get_num_items();
            for (int i = 0; i < num_items; i++) {
                led_control_set_brightness(i, 10);
            }
            wifi_manager_start_sta(g_app_config); // Retry connection
        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            ESP_LOGI(TAG, "STA got IP. Wi-Fi Connected.");
            status_led_set_state(STATUS_LED_NORMAL);
            mdns_service_init(g_app_config->server_name);
            web_server_start(); // Start normal web server

            // Boot sequence: solid-on for 10s then fade-out
            led_control_handle_boot_sequence(BOOT_SEQ_WIFI_CONNECTED);
        }
    }
}


void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Load application configuration
    app_config_load();
    g_app_config = app_config_get();

    // Initialize hardware and services
    led_control_init();
    status_led_init();
    automation_init();
    
    // Initial boot sequence (blinking LEDs at 20% brightness)
    // The actual periodic blinking call will be in the while loop
    
    // Initialize Wi-Fi Manager with event callback
    wifi_manager_init(on_wifi_event, g_app_config->server_name);

    if (!g_app_config->provisioned) {
        ESP_LOGI(TAG, "Device not provisioned. Starting AP for provisioning.");
        g_current_app_mode = MODE_PROVISIONING;
        wifi_manager_start_ap("led-controller-setup"); // SSID from PRD
    } else {
        ESP_LOGI(TAG, "Device provisioned. Attempting to connect to Wi-Fi.");
        g_current_app_mode = MODE_CONNECTING;
        wifi_manager_start_sta(g_app_config);
    }

    // Main loop
    int boot_seq_timer_s = 0;
    bool boot_seq_complete = false;

    while (1) {
        if (g_current_app_mode == MODE_PROVISIONING || g_current_app_mode == MODE_CONNECTING) {
            led_control_handle_boot_sequence(BOOT_SEQ_INITIAL_BLINK);
            vTaskDelay(pdMS_TO_TICKS(500)); // Half-second delay for blink rate
            boot_seq_timer_s = 0; // Reset boot timer until connected
        } else if (g_current_app_mode == MODE_NORMAL && !boot_seq_complete) {
            if (boot_seq_timer_s < 10) {
                // We are in the 10s solid-on period
                boot_seq_timer_s++;
                vTaskDelay(pdMS_TO_TICKS(1000));
            } else {
                // 10s expired, fade out and complete boot sequence
                ESP_LOGI(TAG, "Boot sequence: 10s solid-on expired, fading out.");
                led_control_handle_boot_sequence(BOOT_SEQ_NORMAL_OPERATION);
                boot_seq_complete = true;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000)); // Normal delay
        }
    }
}