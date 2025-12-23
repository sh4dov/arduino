#include "mdns_service.h"
#include "esp_log.h"
#include "mdns.h"
#include "app_config.h"

static const char *TAG = "mdns_service";

static bool s_mdns_initialized = false;

esp_err_t mdns_service_init(const char* device_name)
{
    if (s_mdns_initialized) {
        ESP_LOGW(TAG, "mDNS already initialized, stopping and re-initializing.");
        mdns_service_stop();
    }

    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS Init failed: %s", esp_err_to_name(err));
        return err;
    }

    mdns_hostname_set(device_name);
    mdns_instance_name_set("ESP32-C3 LED Controller");

    mdns_service_add("ESP32-C3-WebServer", "_http", "_tcp", 80, NULL, 0);

    ESP_LOGI(TAG, "mDNS service started. Hostname: %s.local", device_name);
    s_mdns_initialized = true;
    return ESP_OK;
}

void mdns_service_stop(void)
{
    if (s_mdns_initialized) {
        mdns_free();
        ESP_LOGI(TAG, "mDNS service stopped.");
        s_mdns_initialized = false;
    }
}
