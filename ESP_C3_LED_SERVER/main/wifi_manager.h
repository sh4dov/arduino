#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_event.h"
#include "app_config.h" // For app_config_t

typedef enum {
    MODE_PROVISIONING,
    MODE_CONNECTING,
    MODE_NORMAL
} app_mode_t;

typedef struct {
    char ssid[33];        /**< SSID of AP */
    int8_t rssi;          /**< RSSI of AP */
    uint8_t authmode;     /**< Auth mode of AP */
} scanned_wifi_ap_t;


typedef void (*wifi_event_callback_t)(app_mode_t mode, esp_event_base_t event_base, int32_t event_id, void* event_data);

/**
 * @brief Initializes the Wi-Fi manager.
 *
 * This function initializes the TCP/IP stack, default event loop, and Wi-Fi driver.
 * It also registers the internal event handler and stores the provided callback.
 *
 * @param cb A callback function to be called on Wi-Fi events.
 * @param device_name The device name to be used for hostname and mDNS (can be NULL for AP mode only).
 */
void wifi_manager_init(wifi_event_callback_t cb, const char* device_name);

/**
 * @brief Starts Wi-Fi in Access Point (AP) mode for provisioning.
 *
 * @param ssid The SSID for the provisioning AP.
 * @return ESP_OK on success, error otherwise.
 */
esp_err_t wifi_manager_start_ap(const char* ssid);

/**
 * @brief Starts Wi-Fi in Station (STA) mode and attempts to connect to a configured network.
 *
 * @param config Pointer to the application configuration containing Wi-Fi credentials.
 * @return ESP_OK on success, error otherwise.
 */
esp_err_t wifi_manager_start_sta(const app_config_t *config);
void wifi_manager_get_sta_config(char* ssid, size_t ssid_max_len, char* password, size_t password_max_len);

/**
 * @brief Stops the Wi-Fi driver and de-initializes related components.
 * @return ESP_OK on success, error otherwise.
 */
esp_err_t wifi_manager_stop(void);

/**
 * @brief Performs an asynchronous Wi-Fi scan and stores the results.
 * @return ESP_OK on success, error otherwise.
 */
esp_err_t wifi_manager_scan_networks_async(void);

/**
 * @brief Retrieves a copy of the cached Wi-Fi scan results.
 * The caller is responsible for freeing the allocated memory for ap_records.
 *
 * @param ap_records Pointer to a pointer that will hold the array of scanned_wifi_ap_t.
 * @param ap_count Pointer to a uint16_t that will hold the number of APs found.
 * @return ESP_OK on success, error otherwise.
 */
esp_err_t wifi_manager_get_cached_networks(scanned_wifi_ap_t **ap_records, uint16_t *ap_count);

#endif // WIFI_MANAGER_H
