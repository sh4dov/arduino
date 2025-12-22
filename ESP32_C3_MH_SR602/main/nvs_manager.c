#include "nvs_manager.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "nvs_manager";
static const char *NVS_NAMESPACE = "storage";

esp_err_t nvs_manager_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition corrupted or needs to be erased. Erasing NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized successfully.");
    return ret;
}

esp_err_t nvs_manager_load_config(app_config_t *config) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    size_t len;

    len = MAX_DEVICE_NAME_LEN;
    err = nvs_get_str(nvs_handle, "device_name", config->device_name, &len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) goto exit;

    len = MAX_HOST_ADDR_LEN;
    err = nvs_get_str(nvs_handle, "host_addr", config->host_addr, &len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) goto exit;

    len = MAX_WIFI_SSID_LEN;
    err = nvs_get_str(nvs_handle, "wifi_ssid", config->wifi_ssid, &len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) goto exit;

    len = MAX_WIFI_PASS_LEN;
    err = nvs_get_str(nvs_handle, "wifi_pass", config->wifi_password, &len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) goto exit;

    int32_t provisioned = 0;
    err = nvs_get_i32(nvs_handle, "provisioned", &provisioned);
    if (err == ESP_OK) {
        config->provisioned = (bool)provisioned;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->provisioned = false; // Default to not provisioned
    } else {
        goto exit;
    }

    // Use bounded string prints to remain safe even if caller forgot to pre-initialize buffers.
    ESP_LOGI(TAG, "Configuration loaded: Device Name='%.*s', Host Addr='%.*s', SSID='%.*s', Provisioned=%d",
             MAX_DEVICE_NAME_LEN - 1, config->device_name,
             MAX_HOST_ADDR_LEN - 1, config->host_addr,
             MAX_WIFI_SSID_LEN - 1, config->wifi_ssid,
             config->provisioned);

exit:
    nvs_close(nvs_handle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error (%s) reading NVS!", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}

esp_err_t nvs_manager_save_config(const app_config_t *config) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle for writing!", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Saving config: device_name=%s, host_addr=%.*s, wifi_ssid=%s",
             config->device_name, MAX_HOST_ADDR_LEN - 1, config->host_addr,
             config->wifi_ssid);
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "device_name", config->device_name));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "host_addr", config->host_addr));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "wifi_ssid", config->wifi_ssid));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "wifi_pass", config->wifi_password));
    ESP_ERROR_CHECK(nvs_set_i32(nvs_handle, "provisioned", (int32_t)config->provisioned));

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) committing NVS!", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Configuration saved successfully.");
    }
    nvs_close(nvs_handle);
    return err;
}

esp_err_t nvs_manager_factory_reset(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle for factory reset!", esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_all(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) erasing NVS namespace!", esp_err_to_name(err));
    } else {
        ESP_LOGW(TAG, "NVS namespace '%s' erased (Factory Reset).", NVS_NAMESPACE);
    }
    nvs_close(nvs_handle);
    return err;
}
