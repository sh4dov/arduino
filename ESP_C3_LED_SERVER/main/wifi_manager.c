#include "wifi_manager.h"
#include "app_config.h"
#include "web_server.h" 

#include "esp_wifi.h" // Added for wifi_config_t
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "wifi_manager";

#define DEFAULT_SCAN_LIST_SIZE 20

static wifi_event_callback_t g_event_callback = NULL;
static app_mode_t g_current_mode = MODE_PROVISIONING; // Default to provisioning
static char g_device_hostname[32] = {0}; // To store the device hostname

static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;

static bool s_is_wifi_initialized = false;
static esp_event_handler_instance_t s_wifi_event_handler_instance = NULL;
static esp_event_handler_instance_t s_ip_event_handler_instance = NULL;

static wifi_ap_record_t s_ap_info[DEFAULT_SCAN_LIST_SIZE];
static uint16_t s_ap_count = 0;
static bool s_scan_in_progress = false;

// Internal event handler for Wi-Fi events
static void s_wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA_START, trying to connect...");
                esp_wifi_connect();
                g_event_callback(g_current_mode, event_base, event_id, event_data);
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "STA_CONNECTED");
                g_event_callback(g_current_mode, event_base, event_id, event_data);
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "STA_DISCONNECTED. Reason: %d", ((wifi_event_sta_disconnected_t*)event_data)->reason);
                // Call callback before re-connecting so app can react
                g_event_callback(g_current_mode, event_base, event_id, event_data);
                // If not reconnected by callback, retry connection.
                // Callback is responsible for calling wifi_manager_start_sta to retry.
                break;
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "AP_START");
                g_event_callback(g_current_mode, event_base, event_id, event_data);
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "AP_STOP");
                g_event_callback(g_current_mode, event_base, event_id, event_data);
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                {
                    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                    ESP_LOGI(TAG, "Station " MACSTR " joined, AID=%d", MAC2STR(event->mac), event->aid);
                }
                g_event_callback(g_current_mode, event_base, event_id, event_data);
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                {
                    wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                    ESP_LOGI(TAG, "Station " MACSTR " left, AID=%d", MAC2STR(event->mac), event->aid);
                }
                g_event_callback(g_current_mode, event_base, event_id, event_data);
                break;
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "Wi-Fi scan done.");
                s_scan_in_progress = false;
                
                // Retrieve AP records immediately after scan completion
                uint16_t number = DEFAULT_SCAN_LIST_SIZE; 
                esp_err_t err = esp_wifi_scan_get_ap_records(&number, s_ap_info); // number will be updated with actual count
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to get AP records after scan: %s", esp_err_to_name(err));
                    s_ap_count = 0; // Ensure count is 0 on error
                } else {
                    s_ap_count = number; // Store the actual number of found APs
                    ESP_LOGI(TAG, "Found %d access points from scan.", s_ap_count);
                }
                
                g_event_callback(g_current_mode, event_base, event_id, event_data);
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        g_event_callback(g_current_mode, event_base, event_id, event_data);
    }
}

void wifi_manager_init(wifi_event_callback_t cb, const char* device_name)
{
    if (s_is_wifi_initialized) {
        ESP_LOGW(TAG, "Wi-Fi manager already initialized.");
        return;
    }

    g_event_callback = cb;
    if (device_name) {
        strncpy(g_device_hostname, device_name, sizeof(g_device_hostname) - 1);
        g_device_hostname[sizeof(g_device_hostname) - 1] = '\0';
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &s_wifi_event_handler,
                                                        NULL,
                                                        &s_wifi_event_handler_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &s_wifi_event_handler,
                                                        NULL,
                                                        &s_ip_event_handler_instance));

    s_is_wifi_initialized = true;
    ESP_LOGI(TAG, "Wi-Fi Manager initialized.");
}

esp_err_t wifi_manager_start_ap(const char* ssid)
{
    if (!s_is_wifi_initialized) {
        ESP_LOGE(TAG, "Wi-Fi Manager not initialized.");
        return ESP_FAIL;
    }
    
    // If STA was created, destroy it first
    if (s_sta_netif) {
        esp_wifi_stop();
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = NULL;
    }

    if (s_ap_netif == NULL) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
        if (g_device_hostname[0] != '\0') {
            esp_netif_set_hostname(s_ap_netif, g_device_hostname);
        } else {
            // Default AP hostname if no device name provided
            esp_netif_set_hostname(s_ap_netif, "ESP32-AP-Sensor");
        }
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };
    strncpy((char*)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(ssid);

    if (strlen(ssid) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA)); // AP+STA to allow scans
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(40));

    g_current_mode = MODE_PROVISIONING;
    ESP_LOGI(TAG, "AP provisioning mode started with SSID: %s", wifi_config.ap.ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_start_sta(const app_config_t *config)
{
    if (!s_is_wifi_initialized) {
        ESP_LOGE(TAG, "Wi-Fi Manager not initialized.");
        return ESP_FAIL;
    }
    
    // If AP was created, destroy it first
    if (s_ap_netif) {
        esp_wifi_stop(); // Stop current Wi-Fi operation
        esp_netif_destroy(s_ap_netif);
        s_ap_netif = NULL;
    }

    if (s_sta_netif == NULL) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
        if (g_device_hostname[0] != '\0') {
            esp_netif_set_hostname(s_sta_netif, g_device_hostname);
        }
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Minimum security
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    strncpy((char*)wifi_config.sta.ssid, config->wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, config->wifi_password, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(40));

    g_current_mode = MODE_CONNECTING;
    ESP_LOGI(TAG, "STA mode started for SSID: %s", config->wifi_ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_stop(void)
{
    if (!s_is_wifi_initialized) {
        ESP_LOGW(TAG, "Wi-Fi Manager not initialized, nothing to stop.");
        return ESP_OK;
    }

    esp_err_t err = esp_wifi_stop();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGE(TAG, "Failed to stop Wi-Fi: %s", esp_err_to_name(err));
        return err;
    }

    if (s_sta_netif) {
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = NULL;
    }
    if (s_ap_netif) {
        esp_netif_destroy(s_ap_netif);
        s_ap_netif = NULL;
    }

    // Don't de-init the whole driver, just stop active interfaces.
    // The driver remains initialized and ready for a new start_ap/sta call.
    return ESP_OK;
}

esp_err_t wifi_manager_scan_networks_async(void)
{
    if (!s_is_wifi_initialized) {
        ESP_LOGE(TAG, "Wi-Fi Manager not initialized.");
        return ESP_FAIL;
    }
    if (s_scan_in_progress) {
        ESP_LOGW(TAG, "Scan already in progress.");
        return ESP_OK;
    }
    
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.max = 100, // Reduced max scan time
    };
    
    s_scan_in_progress = true;
    ESP_LOGI(TAG, "Starting Wi-Fi scan asynchronously...");
    esp_err_t err = esp_wifi_scan_start(&scan_config, false); // false for asynchronous
    if (err != ESP_OK) {
        s_scan_in_progress = false;
        ESP_LOGE(TAG, "Failed to start Wi-Fi scan: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t wifi_manager_get_cached_networks(scanned_wifi_ap_t **ap_records, uint16_t *ap_count)
{
    if (!s_is_wifi_initialized) {
        ESP_LOGE(TAG, "Wi-Fi Manager not initialized.");
        *ap_count = 0;
        return ESP_FAIL;
    }

    // Always return the currently cached APs.
    // The s_ap_info and s_ap_count are updated when WIFI_EVENT_SCAN_DONE occurs.
    

    if (s_ap_count == 0) {
        *ap_count = 0;
        *ap_records = NULL;
        return ESP_OK;
    }

    *ap_records = (scanned_wifi_ap_t*) malloc(sizeof(scanned_wifi_ap_t) * s_ap_count);
    if (*ap_records == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP records.");
        *ap_count = 0;
        return ESP_ERR_NO_MEM;
    }

    for (int i = 0; i < s_ap_count; i++) {
        strncpy((*ap_records)[i].ssid, (char*)s_ap_info[i].ssid, sizeof((*ap_records)[i].ssid) - 1);
        (*ap_records)[i].ssid[sizeof((*ap_records)[i].ssid) - 1] = '\0';
        (*ap_records)[i].rssi = s_ap_info[i].rssi;
        (*ap_records)[i].authmode = s_ap_info[i].authmode;
    }

    *ap_count = s_ap_count;
    return ESP_OK;
}

void wifi_manager_get_sta_config(char* ssid, size_t ssid_max_len, char* password, size_t password_max_len) {
    app_config_t* config = app_config_get();
    strncpy(ssid, config->wifi_ssid, ssid_max_len - 1);
    ssid[ssid_max_len - 1] = '\0'; // Ensure null termination
    strncpy(password, config->wifi_password, password_max_len - 1);
    password[password_max_len - 1] = '\0'; // Ensure null termination
}