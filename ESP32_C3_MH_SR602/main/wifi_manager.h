
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"
#include "app_state.h"
#include "esp_wifi.h" // Include esp_wifi.h for wifi_auth_mode_t and other Wi-Fi types
#include "nvs_manager.h" // Include nvs_manager.h for app_config_t

// Structure to hold information about a scanned Wi-Fi access point for external use (e.g., web server)
typedef struct {
    char ssid[33];         // SSID of the AP
    int8_t rssi;           // RSSI of the AP
    wifi_auth_mode_t authmode; // Authentication mode
} scanned_wifi_ap_t;

// Callback type for Wi-Fi events (e.g., connected, disconnected)
typedef void (*wifi_event_callback_t)(app_mode_t mode, esp_event_base_t event_base, int32_t event_id, void* event_data);

// Initialize Wi-Fi driver and event loop
void wifi_manager_init(wifi_event_callback_t cb, const char* device_name);

// Start Wi-Fi in Access Point (AP) mode for provisioning
esp_err_t wifi_manager_start_ap(const char* ssid);

// Start Wi-Fi in Station (STA) mode to connect to an existing network
esp_err_t wifi_manager_start_sta(const app_config_t *config);

// Stop Wi-Fi
esp_err_t wifi_manager_stop(void);

// Scan for available Wi-Fi networks (updates internal cache)
esp_err_t wifi_manager_scan_networks(void);

// Trigger a Wi-Fi scan in a background task (updates internal cache).
// Safe to call from event handlers; returns immediately.
esp_err_t wifi_manager_scan_networks_async(void);

// Function to get cached Wi-Fi networks
esp_err_t wifi_manager_get_cached_networks(scanned_wifi_ap_t **ap_records, uint16_t *ap_count);

#endif // WIFI_MANAGER_H

