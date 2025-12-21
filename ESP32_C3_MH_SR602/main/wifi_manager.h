#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"
#include "app_state.h"

// Callback type for Wi-Fi events (e.g., connected, disconnected)
typedef void (*wifi_event_callback_t)(app_mode_t mode, esp_event_base_t event_base, int32_t event_id, void* event_data);

// Initialize Wi-Fi driver and event loop
void wifi_manager_init(wifi_event_callback_t cb, const char* device_name);

// Start Wi-Fi in Access Point (AP) mode for provisioning
esp_err_t wifi_manager_start_ap(const char* ssid);

// Start Wi-Fi in Station (STA) mode to connect to an existing network
esp_err_t wifi_manager_start_sta(const char* ssid, const char* password);

// Stop Wi-Fi
esp_err_t wifi_manager_stop(void);

#endif // WIFI_MANAGER_H
