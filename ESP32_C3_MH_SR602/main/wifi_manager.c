#include "wifi_manager.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_netif.h" // For esp_netif_set_hostname
#include "lwip/ip_addr.h" // Add this line for IP2STR

static const char *TAG = "wifi_manager";
static wifi_event_callback_t s_wifi_event_callback = NULL;
static esp_netif_t *s_esp_netif_sta = NULL; // Keep a reference to STA netif

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
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Reconnection logic is handled in main.c to implement continuous retries.
        // This handler will simply forward the event.
        ESP_LOGI(TAG, "station: disconnected");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }

    // Forward the event to the registered callback in main
    if (s_wifi_event_callback) {
        // Determine current mode - for now, we infer it. This can be improved.
        wifi_mode_t mode;
        esp_wifi_get_mode(&mode);
        app_mode_t app_mode = (mode == WIFI_MODE_AP) ? MODE_PROVISIONING : MODE_CONNECTING; // Simplified
        s_wifi_event_callback(app_mode, event_base, event_id, event_data);
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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s", ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_start_sta(const char* ssid, const char* password) {
    if (ssid == NULL || ssid[0] == '\0') {
        ESP_LOGE(TAG, "Cannot start STA with empty SSID");
        return ESP_ERR_INVALID_ARG;
    }
    if (password == NULL) {
        password = "";
    }
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t)); // Zero out the struct to ensure null termination

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) -1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    return ESP_OK;
}

esp_err_t wifi_manager_stop(void) {
    ESP_LOGI(TAG, "Stopping Wi-Fi");
    return esp_wifi_stop();
}