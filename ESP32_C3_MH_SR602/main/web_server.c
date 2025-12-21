#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <string.h>
#include <ctype.h>
#include "nvs_manager.h" // For app_config_t and saving config
#include "esp_system.h"  // For esp_restart()
#include "app_state.h"
#include "motion_sensor.h" // For motion_sensor_get_status()
#include "cJSON.h"         // For proper JSON generation

// Extern variables to access global state from main.c
extern app_config_t g_app_config;
extern app_mode_t g_app_mode;

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

// --- Normal Mode Handlers ---

// HTML for the status page in Normal Mode
const char* NORMAL_HTML_PAGE = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta http-equiv="refresh" content="5">
    <title>Sensor Status</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f4; color: #333; display: flex; justify-content: center; align-items: center; height: 90vh; }
        .container { background-color: #fff; padding: 40px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); text-align: center; }
        h1 { color: #0056b3; margin-bottom: 15px; }
        p { font-size: 24px; }
        .status-detected { color: #d9534f; font-weight: bold; }
        .status-none { color: #5cb85c; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Device: %.*s</h1>
        <p>Motion Status: <span class="%s">%s</span></p>
    </div>
</body>
</html>
)rawliteral";

// URI handler for the root path (/) in Normal Mode
static esp_err_t normal_root_get_handler(httpd_req_t *req) {
    char html_response[1024];
    motion_status_t motion_state = motion_sensor_get_status();
    
    snprintf(html_response, sizeof(html_response), NORMAL_HTML_PAGE,
             MAX_DEVICE_NAME_LEN - 1, g_app_config.device_name,
             (motion_state == MOTION_DETECTED) ? "status-detected" : "status-none",
             (motion_state == MOTION_DETECTED) ? "Detected" : "None");

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// URI handler for /api/status in Normal Mode
static esp_err_t normal_api_status_get_handler(httpd_req_t *req) {
    motion_status_t motion_state = motion_sensor_get_status();

    // Use cJSON to properly generate JSON with escaped strings
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object for /api/status");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal error");
        return ESP_FAIL;
    }

    cJSON_AddStringToObject(root, "name", g_app_config.device_name);
    cJSON_AddNumberToObject(root, "status", (motion_state == MOTION_DETECTED) ? 1 : 0);

    const char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON for /api/status");
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal error");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));

    cJSON_free((void *)json_string);
    cJSON_Delete(root);
    return ESP_OK;
}


// --- Provisioning Mode Handlers ---

// HTML for the provisioning form
const char* PROV_HTML_FORM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32-C3 Sensor Setup</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f4; color: #333; }
        .container { background-color: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); max-width: 500px; margin: auto; }
        h1 { color: #0056b3; text-align: center; margin-bottom: 25px; }
        label { display: block; margin-bottom: 8px; font-weight: bold; }
        input[type="text"], input[type="password"] {
            width: calc(100%% - 22px);
            padding: 10px;
            margin-bottom: 15px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 16px;
        }
        input[type="submit"] {
            background-color: #28a745;
            color: white;
            padding: 12px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            width: 100%%;
        }
        input[type="submit"]:hover {
            background-color: #218838;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Motion Sensor Setup</h1>
        <form action="/save" method="post">
            <label for="device_name">Device Name:</label>
            <input type="text" id="device_name" name="device_name" value="%.*s" required>

            <label for="host_ip">Host Server IP:</label>
            <input type="text" id="host_ip" name="host_ip" value="%.*s" pattern="^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$" required>

            <label for="wifi_ssid">Wi-Fi SSID:</label>
            <input type="text" id="wifi_ssid" name="wifi_ssid" required>

            <label for="wifi_password">Wi-Fi Password:</label>
            <input type="password" id="wifi_password" name="wifi_password">

            <input type="submit" value="Save Configuration">
        </form>
    </div>
</body>
</html>
)rawliteral";


// URI handler for the root path (/) in Provisioning Mode
static esp_err_t prov_root_get_handler(httpd_req_t *req) {
    char html_response[2048]; // Buffer for dynamic HTML
    snprintf(html_response, sizeof(html_response), PROV_HTML_FORM, 
             MAX_DEVICE_NAME_LEN - 1, (g_app_config.provisioned ? g_app_config.device_name : ""),
             MAX_HOST_IP_LEN - 1, (g_app_config.provisioned ? g_app_config.host_ip : ""));

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// URL decode helper function
static void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Basic hostname validation (RFC 952/1123 compliant)
static bool is_valid_hostname(const char *name) {
    if (name == NULL) return false;
    
    size_t len = strlen(name);
    // Check length constraints (1-63 characters per label, but we'll limit to 31 for device names)
    if (len == 0 || len > 31) return false;
    
    // Cannot start or end with hyphen
    if (name[0] == '-' || name[len-1] == '-') return false;
    
    // Check all characters are alphanumeric or hyphen
    for (size_t i = 0; i < len; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '-') {
            return false;
        }
    }
    return true;
}

// IP address validation (IPv4)
static bool is_valid_ip(const char *ip) {
    if (ip == NULL || strlen(ip) == 0) return false;
    
    int nums[4];
    int count = sscanf(ip, "%d.%d.%d.%d", &nums[0], &nums[1], &nums[2], &nums[3]);
    
    if (count != 4) return false;
    
    for (int i = 0; i < 4; i++) {
        if (nums[i] < 0 || nums[i] > 255) {
            return false;
        }
    }
    
    // Additional check: ensure no extra characters
    char reconstructed[16];
    snprintf(reconstructed, sizeof(reconstructed), "%d.%d.%d.%d", nums[0], nums[1], nums[2], nums[3]);
    return strcmp(ip, reconstructed) == 0;
}

// URI handler for /save (POST)
static esp_err_t prov_save_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret, remaining = req->content_len;
    
    if (remaining > sizeof(buf) -1) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    app_config_t new_config;
    // It's good practice to clear memory, especially for credentials
    memset(&new_config, 0, sizeof(app_config_t));
    
    char param_buf[100];
    char decoded_buf[100];
    
    // Parse and URL-decode form data
    if (httpd_query_key_value(buf, "device_name", param_buf, sizeof(param_buf)) == ESP_OK) {
        url_decode(decoded_buf, param_buf);
        strncpy(new_config.device_name, decoded_buf, sizeof(new_config.device_name) - 1);
    }
    if (httpd_query_key_value(buf, "host_ip", param_buf, sizeof(param_buf)) == ESP_OK) {
        url_decode(decoded_buf, param_buf);
        strncpy(new_config.host_ip, decoded_buf, sizeof(new_config.host_ip) - 1);
    }
    if (httpd_query_key_value(buf, "wifi_ssid", param_buf, sizeof(param_buf)) == ESP_OK) {
        url_decode(decoded_buf, param_buf);
        strncpy(new_config.wifi_ssid, decoded_buf, sizeof(new_config.wifi_ssid) - 1);
    }
    if (httpd_query_key_value(buf, "wifi_password", param_buf, sizeof(param_buf)) == ESP_OK) {
        url_decode(decoded_buf, param_buf);
        strncpy(new_config.wifi_password, decoded_buf, sizeof(new_config.wifi_password) - 1);
    }
    
    // Server-side validation
    if (strlen(new_config.wifi_ssid) == 0 || strlen(new_config.device_name) == 0 || strlen(new_config.host_ip) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing required fields!");
        return ESP_FAIL;
    }

    if (!is_valid_hostname(new_config.device_name)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid device name. Must be 1-31 characters, alphanumeric and hyphen only, cannot start/end with hyphen.");
        return ESP_FAIL;
    }

    if (!is_valid_ip(new_config.host_ip)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid IP address format. Use format: xxx.xxx.xxx.xxx");
        return ESP_FAIL;
    }

    new_config.provisioned = true;
    nvs_manager_save_config(&new_config);

    const char* success_html = "<h1>Configuration Saved! Device rebooting...</h1>";
    httpd_resp_send(req, success_html, HTTPD_RESP_USE_STRLEN);

    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

    return ESP_OK;
}

// --- Server Control ---

esp_err_t web_server_start(void) {
    if (server) {
        ESP_LOGW(TAG, "Server already running. Please stop it first.");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.stack_size = 8192;
    
    ESP_LOGI(TAG, "Starting server for mode %d on port: '%d'", g_app_mode, config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers for mode %d", g_app_mode);

        if (g_app_mode == MODE_PROVISIONING) {
            httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = prov_root_get_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &root_uri);

            httpd_uri_t save_uri = { .uri = "/save", .method = HTTP_POST, .handler = prov_save_post_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &save_uri);

        } else if (g_app_mode == MODE_NORMAL) {
            httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = normal_root_get_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &root_uri);

            httpd_uri_t api_status_uri = { .uri = "/api/status", .method = HTTP_GET, .handler = normal_api_status_get_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &api_status_uri);
        }
        
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Error starting server!");
    return ESP_FAIL;
}

esp_err_t web_server_stop(void) {
    if (server) {
        ESP_LOGI(TAG, "Stopping web server");
        httpd_stop(server);
        server = NULL;
    }
    return ESP_OK;
}
