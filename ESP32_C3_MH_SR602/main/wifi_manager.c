
#include "wifi_manager.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_netif.h" // For esp_netif_set_hostname
#include "lwip/ip_addr.h" // Add this line for IP2STR
#include "app_state.h" // Include app_state.h for g_app_mode
#include <time.h> // For time_t

#define DEFAULT_SCAN_LIST_SIZE 20 // Max number of APs to store in the scan list

static const char *TAG = "wifi_manager";
static wifi_event_callback_t s_wifi_event_callback = NULL;
static esp_netif_t *s_esp_netif_sta = NULL; // Keep a reference to STA netif

// Global cache for scanned networks
static scanned_wifi_ap_t *s_cached_ap_records = NULL;
static uint16_t s_cached_ap_count = 0;
static time_t s_last_scan_time = 0; // To store when the last scan occurred
static bool s_wifi_started = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "station: start, trying to connect...");
        // Only attempt to connect if the device has been provisioned
        if (g_app_config.provisioned) {
            esp_wifi_connect();
        } else {
            ESP_LOGI(TAG, "Device not provisioned, not auto-connecting STA.");
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Reconnection logic is handled in main.c to implement continuous retries.
        // This handler will simply forward the event.
        ESP_LOGI(TAG, "station: disconnected");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }

    // Forward the event to the registered callback in main, using the actual g_app_mode
    if (s_wifi_event_callback) {
        s_wifi_event_callback(g_app_mode, event_base, event_id, event_data);
    }
}

void wifi_manager_init(wifi_event_callback_t cb, const char* device_name) {
    s_wifi_event_callback = cb;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default network interfaces for both STA and AP
    s_esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_t *s_esp_netif_ap = esp_netif_create_default_wifi_ap();

    // Set the hostname for the STA interface if a device name is provided
    if (device_name && strlen(device_name) > 0) {
        ESP_ERROR_CHECK(esp_netif_set_hostname(s_esp_netif_sta, device_name));
        ESP_LOGI(TAG, "STA hostname set to: %s", device_name);
        // Also set the AP hostname for consistency, using the same device name
        ESP_ERROR_CHECK(esp_netif_set_hostname(s_esp_netif_ap, device_name));
        ESP_LOGI(TAG, "AP hostname set to: %s", device_name);
    } else {
        // If no device name provided (e.g., in provisioning mode), set a default hostname for AP
        ESP_ERROR_CHECK(esp_netif_set_hostname(s_esp_netif_ap, "ESP32-AP-Sensor"));
        ESP_LOGI(TAG, "AP hostname set to default: ESP32-AP-Sensor");
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_set_max_tx_power(40);
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Note: actual Wi-Fi mode + start is handled by wifi_manager_start_ap / wifi_manager_start_sta.

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    // Note: We don't register for IP_EVENT_AP_STAIPASSIGNED to avoid crash issues
}

esp_err_t wifi_manager_start_ap(const char* ssid) {
    if (ssid == NULL || ssid[0] == '\0') {
        ESP_LOGE(TAG, "Cannot start AP with empty SSID");
        return ESP_ERR_INVALID_ARG;
    }
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ssid),
            .channel = 1, // Default channel
            .max_connection = 4, // Max 4 clients
            .authmode = WIFI_AUTH_OPEN
        },
    };
    strcpy((char*)wifi_config.ap.ssid, ssid);

    // Use AP+STA so we can scan while the provisioning AP is running.
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    // Mark started *before* esp_wifi_start(): AP_START/STA_START events can fire
    // before esp_wifi_start() returns.
    s_wifi_started = true;
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s", ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_start_sta(const app_config_t *config) {
    if (config == NULL || strlen(config->wifi_ssid) == 0) {
        ESP_LOGE(TAG, "Cannot start STA with empty SSID or NULL config");
        return ESP_ERR_INVALID_ARG;
    }
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t)); // Zero out the struct to ensure null termination

    strncpy((char*)wifi_config.sta.ssid, config->wifi_ssid, sizeof(wifi_config.sta.ssid) -1);
    strncpy((char*)wifi_config.sta.password, config->wifi_password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // Mark started *before* esp_wifi_start(): events can fire before it returns.
    s_wifi_started = true;
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished. Connecting to SSID: %s", config->wifi_ssid);
    return ESP_OK;
}
esp_err_t wifi_manager_stop(void) {
    ESP_LOGI(TAG, "Stopping Wi-Fi");
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_OK) {
        s_wifi_started = false;
    }
    return err;
}

esp_err_t wifi_manager_get_cached_networks(scanned_wifi_ap_t **ap_records_out, uint16_t *ap_count_out) {
    if (s_cached_ap_records == NULL || s_cached_ap_count == 0) {
        return ESP_ERR_NOT_FOUND; // Indicate no cached data
    }

    // Allocate memory for the copy
    *ap_records_out = (scanned_wifi_ap_t *)malloc(sizeof(scanned_wifi_ap_t) * s_cached_ap_count);
    if (*ap_records_out == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for cached AP records copy.");
        return ESP_ERR_NO_MEM;
    }

    // Copy the cached data
    memcpy(*ap_records_out, s_cached_ap_records, sizeof(scanned_wifi_ap_t) * s_cached_ap_count);
    *ap_count_out = s_cached_ap_count;

    return ESP_OK;
}
esp_err_t wifi_manager_scan_networks(void) {
    esp_err_t ret = ESP_OK;
    uint16_t number = DEFAULT_SCAN_LIST_SIZE; // Max APs to store

    // Ensure Wi-Fi is started before scanning.
    if (!s_wifi_started) {
        ESP_LOGW(TAG, "Wi-Fi not started yet; cannot scan.");
        return ESP_ERR_WIFI_NOT_STARTED;
    }

    // Ensure current mode includes STA (scan requires STA capability).
    wifi_mode_t current_mode = WIFI_MODE_NULL;
    if (esp_wifi_get_mode(&current_mode) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get Wi-Fi mode");
        return ESP_FAIL;
    }
    if (current_mode != WIFI_MODE_STA && current_mode != WIFI_MODE_APSTA) {
        ESP_LOGW(TAG, "Wi-Fi mode (%d) does not support scanning; expected STA or APSTA.", current_mode);
        return ESP_ERR_NOT_SUPPORTED;
    }

    // Allocate memory for the raw ESP-IDF AP records
    wifi_ap_record_t *ap_info_esp = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * number);
    if (ap_info_esp == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP info.");
        ret = ESP_ERR_NO_MEM;
        goto cleanup; // Use goto for error handling
    }

    uint16_t actual_scan_count = 0;
    ret = esp_wifi_scan_start(NULL, true); // Blocking scan
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi scan: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    ret = esp_wifi_scan_get_ap_records(&number, ap_info_esp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP records: %s", esp_err_to_name(ret));
        goto cleanup;
    }
    actual_scan_count = number;

    // Free existing cache if any
    if (s_cached_ap_records) {
        free(s_cached_ap_records);
        s_cached_ap_records = NULL;
        s_cached_ap_count = 0;
    }

    // Allocate memory for our custom scanned_wifi_ap_t and copy to cache
    s_cached_ap_records = (scanned_wifi_ap_t *)malloc(sizeof(scanned_wifi_ap_t) * actual_scan_count);
    if (s_cached_ap_records == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for cached AP records.");
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    s_cached_ap_count = actual_scan_count;

    for (int i = 0; i < actual_scan_count; i++) {
        strncpy(s_cached_ap_records[i].ssid, (char *)ap_info_esp[i].ssid, sizeof(s_cached_ap_records[i].ssid) - 1);
        s_cached_ap_records[i].ssid[sizeof(s_cached_ap_records[i].ssid) - 1] = '\0'; // Ensure null-termination
        s_cached_ap_records[i].rssi = ap_info_esp[i].rssi;
        s_cached_ap_records[i].authmode = ap_info_esp[i].authmode;
    }

    s_last_scan_time = time(NULL); // Update last scan time

cleanup:
    if (ap_info_esp) {
        free(ap_info_esp);
    }

    return ret;
}

static void wifi_scan_task(void *arg) {
    (void)arg;
    esp_err_t err = wifi_manager_scan_networks();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Async Wi-Fi scan failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Async Wi-Fi scan complete. Cached %u APs.", (unsigned)s_cached_ap_count);
    }
    vTaskDelete(NULL);
}

esp_err_t wifi_manager_scan_networks_async(void) {
    // Avoid spawning a scan task if Wi-Fi is not started.
    if (!s_wifi_started) return ESP_ERR_WIFI_NOT_STARTED;

    BaseType_t ok = xTaskCreate(wifi_scan_task, "wifi_scan", 4096, NULL, 5, NULL);
    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}