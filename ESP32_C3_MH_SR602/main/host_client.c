#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "app_state.h"
#include "host_client.h"
#include "nvs_manager.h" // For app_config_t definition

// Configuration constants per PRD requirements
#define MAX_RETRIES 3           // PRD: "retry up to 3 times"
#define RETRY_DELAY_MS 5000     // 5 second delay between retry attempts
#define POST_URL_FORMAT "http://%s/api/motionSensor"
#define HTTP_TIMEOUT_MS 5000    // 5 second timeout for HTTP requests

static const char *TAG = "host_client";

// Extern declaration for the global application configuration
extern app_config_t g_app_config;

// Struct to pass data to the HTTP task
typedef struct {
    motion_status_t status;
} http_task_params_t;

// HTTP Client Event Handler
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            // header_key/header_value can be NULL depending on the event/state; guard to avoid printf crashes.
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s",
                     evt->header_key ? evt->header_key : "(null)",
                     evt->header_value ? evt->header_value : "(null)");
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

// Task to send HTTP POST request
static void http_post_task(void *pvParameters) {
    http_task_params_t *params = (http_task_params_t *)pvParameters;
    motion_status_t status = params->status;
    free(params); // Free the parameters struct immediately

    char post_url[sizeof(POST_URL_FORMAT) + MAX_HOST_IP_LEN];
    snprintf(post_url, sizeof(post_url), POST_URL_FORMAT, g_app_config.host_ip);

    ESP_LOGI(TAG, "Sending motion update to %s. State: %d", post_url, status);

    // Create JSON payload
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object.");
        vTaskDelete(NULL);
        return;
    }
    cJSON_AddStringToObject(root, "sensor", g_app_config.device_name);
    cJSON_AddNumberToObject(root, "state", (status == MOTION_DETECTED) ? 1 : 0);
    const char *post_data = cJSON_PrintUnformatted(root);
    if (post_data == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON to string.");
        cJSON_Delete(root);
        vTaskDelete(NULL);
        return;
    }

    esp_http_client_config_t config = {
        .url = post_url,
        .event_handler = _http_event_handler,
        .method = HTTP_METHOD_POST,
        .timeout_ms = HTTP_TIMEOUT_MS,
    };

    // Retry logic: attempt up to MAX_RETRIES times with RETRY_DELAY_MS between failures
    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, post_data, strlen(post_data));

        ESP_LOGI(TAG, "Attempt %d/%d: Sending POST to %s", attempt, MAX_RETRIES, post_url);
        
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            ESP_LOGI(TAG, "HTTP POST successful on attempt %d. HTTP Status: %d", attempt, status_code);
            esp_http_client_cleanup(client);
            cJSON_free((void *)post_data);
            cJSON_Delete(root);
            vTaskDelete(NULL);
            return;
        } else {
            ESP_LOGW(TAG, "HTTP POST failed on attempt %d/%d: %s", attempt, MAX_RETRIES, esp_err_to_name(err));
            esp_http_client_cleanup(client);
            
            if (attempt < MAX_RETRIES) {
                ESP_LOGI(TAG, "Retrying in %d ms...", RETRY_DELAY_MS);
                vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
            }
        }
    }

    ESP_LOGE(TAG, "Failed to send motion update after %d attempts. Host may be unreachable.", MAX_RETRIES);
    cJSON_free((void *)post_data);
    cJSON_Delete(root);
    vTaskDelete(NULL); // Delete task after all retries fail
}

void host_client_send_motion_update(motion_status_t status) {
    if (strlen(g_app_config.host_ip) == 0) {
        ESP_LOGW(TAG, "Host IP not configured. Skipping motion update.");
        return;
    }

    // Create a parameter struct to pass to the task
    http_task_params_t *params = malloc(sizeof(http_task_params_t));
    if (params == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for http_post_task parameters.");
        return;
    }
    params->status = status;

    // Create a task to handle the blocking network call
    xTaskCreate(http_post_task, "http_post_task", 4096, params, 5, NULL);
}
