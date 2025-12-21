#include "mdns_service.h"
#include "mdns.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mdns_service";
static bool s_mdns_initialized = false; // Track mDNS initialization state

esp_err_t mdns_service_init(const char* device_name) {
    if (!device_name || strlen(device_name) == 0) {
        ESP_LOGE(TAG, "Cannot initialize mDNS with an empty device name.");
        return ESP_ERR_INVALID_ARG;
    }

    // If already initialized, stop it first
    if (s_mdns_initialized) {
        ESP_LOGW(TAG, "mDNS already initialized. Stopping and reinitializing...");
        mdns_service_stop();
    }

    // Initialize mDNS
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGE(TAG, "mDNS Init failed: %s", esp_err_to_name(err));
        return err;
    }

    // Set hostname
    err = mdns_hostname_set(device_name);
    if (err) {
        ESP_LOGE(TAG, "mDNS hostname set failed: %s", esp_err_to_name(err));
        mdns_free(); // Clean up on error
        return err;
    }
    ESP_LOGI(TAG, "mDNS hostname set to: %s", device_name);

    // Set default instance
    err = mdns_instance_name_set("ESP32-C3 Motion Sensor");
    if (err) {
        ESP_LOGE(TAG, "mDNS instance name set failed: %s", esp_err_to_name(err));
        mdns_free(); // Clean up on error
        return err;
    }
    
    // Add service
    err = mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    if (err) {
        ESP_LOGE(TAG, "mDNS service add failed: %s", esp_err_to_name(err));
        mdns_free(); // Clean up on error
        return err;
    }

    s_mdns_initialized = true;
    ESP_LOGI(TAG, "mDNS service initialized successfully for http://%s.local", device_name);
    return ESP_OK;
}

void mdns_service_stop(void) {
    if (s_mdns_initialized) {
        mdns_free();
        s_mdns_initialized = false;
        ESP_LOGI(TAG, "mDNS service stopped.");
    } else {
        ESP_LOGD(TAG, "mDNS service not running, nothing to stop.");
    }
}
