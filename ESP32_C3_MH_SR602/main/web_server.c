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
#include "wifi_manager.h"  // For wifi_manager_scan_networks and wifi_ap_record_t


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
        .edit-button {
            background-color: #007bff;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            margin-top: 20px;
            text-decoration: none;
            display: inline-block;
        }
        .edit-button:hover {
            background-color: #0056b3;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Device: %.*s</h1>
        <p>Motion Status: <span class="%s">%s</span></p>
        <a href="/edit" class="edit-button">Edit Configuration</a>
    </div>
</body>
</html>
)rawliteral";

// URI handler for the root path (/) in Normal Mode
static esp_err_t normal_root_get_handler(httpd_req_t *req) {
    char html_response[2048]; // Increased buffer size for safety
    motion_status_t motion_state = motion_sensor_get_status();

    ESP_LOGI(TAG, "Serving normal root page. Device Name: %s, Motion Status: %s",
             g_app_config.device_name, (motion_state == MOTION_DETECTED) ? "Detected" : "None");

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
        input[type="text"], input[type="password"], select {
            width: calc(100%% - 22px);
            padding: 10px;
            margin-bottom: 15px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 16px;
        }
        button, input[type="submit"] {
            background-color: #28a745;
            color: white;
            padding: 12px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            width: 100%%;
            margin-bottom: 10px;
        }
        button.secondary {
            background-color: #007bff;
        }
        button:hover, input[type="submit"]:hover {
            background-color: #218838;
        }
        button.secondary:hover {
            background-color: #0056b3;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Motion Sensor Setup</h1>
        <form action="%s" method="post">
            <label for="device_name">Device Name:</label>
            <input type="text" id="device_name" name="device_name" value="%.*s" required>

            <label for="host_addr">Host Server Address (IP or Hostname:Port):</label>
            <input type="text" id="host_addr" name="host_addr" value="%.*s" required>

            <label for="wifi_networks">Available Wi-Fi Networks:</label>
            <select id="wifi_networks" onchange="document.getElementById('wifi_ssid').value = this.value;">
                <option value="">-- Select a network --</option>
                %s
            </select>
            <button type="button" class="secondary" onclick="refreshWifiList()">Scan Wi-Fi</button>

            <label for="wifi_ssid">Wi-Fi SSID:</label>
            <input type="text" id="wifi_ssid" name="wifi_ssid" value="%.*s" required>

            <label for="wifi_password">Wi-Fi Password:</label>
            <input type="password" id="wifi_password" name="wifi_password" value="%.*s">

            <input type="submit" value="Save Configuration">
        </form>
    </div>

    <script>
        // Function to perform a fresh scan
        async function refreshWifiList() {
            const selectElement = document.getElementById('wifi_networks');
            selectElement.innerHTML = '<option value="">-- Scanning... --</option>';
            selectElement.disabled = true;

            try {
                const response = await fetch('/scan_wifi'); // This triggers a live scan
                if (!response.ok) {
                    throw new Error(`scan_wifi failed: ${response.status}`);
                }
                const data = await response.json();

                selectElement.innerHTML = '<option value="">-- Select a network --</option>'; // Clear and add default
                data.forEach(ap => {
                    const option = document.createElement('option');
                    option.value = ap.ssid;
                    option.textContent = `${ap.ssid} (RSSI: ${ap.rssi}, Auth: ${ap.authmode})`;
                    selectElement.appendChild(option);
                });
                if (!data || data.length === 0) {
                    selectElement.innerHTML = '<option value="">-- No networks found --</option>';
                }
            } catch (error) {
                console.error('Error scanning Wi-Fi:', error);
                selectElement.innerHTML = '<option value="">-- Scan failed --</option>';
            } finally {
                selectElement.disabled = false;
            }
        }
    </script>
</body>
</html>)rawliteral";

static size_t html_escape(char *dst, size_t dst_len, const char *src) {
    // Escapes only what we need for putting SSIDs into HTML attributes/text.
    // Returns bytes written (excluding final NUL).
    if (!dst || dst_len == 0) return 0;
    size_t w = 0;
    for (const char *p = src ? src : ""; *p; p++) {
        const char *rep = NULL;
        switch (*p) {
            case '&': rep = "&amp;"; break;
            case '<': rep = "&lt;"; break;
            case '>': rep = "&gt;"; break;
            case '"': rep = "&quot;"; break;
            default:  rep = NULL; break;
        }
        if (rep) {
            size_t rlen = strlen(rep);
            if (w + rlen >= dst_len) break;
            memcpy(dst + w, rep, rlen);
            w += rlen;
        } else {
            if (w + 1 >= dst_len) break;
            dst[w++] = *p;
        }
    }
    dst[w] = '\0';
    return w;
}

static void build_cached_wifi_options(char *out, size_t out_len) {
    if (!out || out_len == 0) return;
    out[0] = '\0';

    scanned_wifi_ap_t *ap_records = NULL;
    uint16_t ap_count = 0;
    esp_err_t err = wifi_manager_get_cached_networks(&ap_records, &ap_count);
    if (err != ESP_OK || !ap_records || ap_count == 0) {
        if (ap_records) free(ap_records);
        return;
    }

    size_t w = 0;
    for (int i = 0; i < ap_count; i++) {
        char ssid_esc[96];
        html_escape(ssid_esc, sizeof(ssid_esc), ap_records[i].ssid);
        int n = snprintf(out + w, out_len - w,
                         "<option value=\"%s\">%s (RSSI: %d)</option>\n",
                         ssid_esc, ssid_esc, (int)ap_records[i].rssi);
        if (n < 0) break;
        if ((size_t)n >= out_len - w) { // truncated
            break;
        }
        w += (size_t)n;
    }

    free(ap_records);
}


// URI handler for the root path (/) in Provisioning Mode
static esp_err_t prov_root_get_handler(httpd_req_t *req) {
    // Avoid large stack allocations inside the httpd task.
    char *wifi_options = (char *)calloc(1, 2048);
    char *html_response = (char *)calloc(1, 8192);
    if (!wifi_options || !html_response) {
        free(wifi_options);
        free(html_response);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }

    build_cached_wifi_options(wifi_options, 2048);
    snprintf(html_response, 8192, PROV_HTML_FORM,
             "/save", // Form action for provisioning
             MAX_DEVICE_NAME_LEN - 1, (g_app_config.provisioned ? g_app_config.device_name : ""),
             MAX_HOST_ADDR_LEN - 1, (g_app_config.provisioned ? g_app_config.host_addr : ""),
             wifi_options,
             MAX_WIFI_SSID_LEN - 1, (g_app_config.provisioned ? g_app_config.wifi_ssid : ""),
             MAX_WIFI_PASS_LEN - 1, (g_app_config.provisioned ? g_app_config.wifi_password : ""));

    // Avoid browser caching an older version of the provisioning page.
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    free(wifi_options);
    free(html_response);
    return ESP_OK;
}

// URI handler for /get_cached_wifi_list (GET)
static esp_err_t get_cached_wifi_list_handler(httpd_req_t *req) {
    scanned_wifi_ap_t *ap_records = NULL;
    uint16_t ap_count = 0;
    esp_err_t err = wifi_manager_get_cached_networks(&ap_records, &ap_count);

    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to get cached Wi-Fi networks: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get cached Wi-Fi networks");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "/get_cached_wifi_list -> %u APs (err=%s)", (unsigned)ap_count, esp_err_to_name(err));

    cJSON *root = cJSON_CreateArray();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON array for /get_cached_wifi_list");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal error");
        return ESP_FAIL;
    }

    // Populate JSON only if ap_records exist
    if (ap_records) {
        for (int i = 0; i < ap_count; i++) {
            cJSON *ap = cJSON_CreateObject();
            if (ap == NULL) {
                ESP_LOGE(TAG, "Failed to create JSON object for cached AP record");
                cJSON_Delete(root);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal error");
                return ESP_FAIL;
            }
            cJSON_AddStringToObject(ap, "ssid", ap_records[i].ssid);
            cJSON_AddNumberToObject(ap, "rssi", ap_records[i].rssi);
            const char *auth_mode_str;
            switch (ap_records[i].authmode) {
                case WIFI_AUTH_OPEN:        auth_mode_str = "OPEN"; break;
                case WIFI_AUTH_WEP:         auth_mode_str = "WEP"; break;
                case WIFI_AUTH_WPA_PSK:     auth_mode_str = "WPA_PSK"; break;
                case WIFI_AUTH_WPA2_PSK:    auth_mode_str = "WPA2_PSK"; break;
                case WIFI_AUTH_WPA_WPA2_PSK: auth_mode_str = "WPA_WPA2_PSK"; break;
                case WIFI_AUTH_WPA2_ENTERPRISE: auth_mode_str = "WPA2_ENTERPRISE"; break;
                case WIFI_AUTH_WPA3_PSK:    auth_mode_str = "WPA3_PSK"; break;
                case WIFI_AUTH_WPA2_WPA3_PSK: auth_mode_str = "WPA2_WPA3_PSK"; break;
                case WIFI_AUTH_WAPI_PSK:    auth_mode_str = "WAPI_PSK"; break;
                default:                    auth_mode_str = "UNKNOWN"; break;
            }
            cJSON_AddStringToObject(ap, "authmode", auth_mode_str);
            cJSON_AddItemToArray(root, ap);
        }
    }

    const char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON for /get_cached_wifi_list");
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal error");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));

    cJSON_free((void *)json_string);
    cJSON_Delete(root);
    if (ap_records) {
        free(ap_records); // Free the memory allocated by wifi_manager_get_cached_networks
    }
    return ESP_OK;
}
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



// Helper function to handle saving config and rebooting
static esp_err_t _save_config_and_reboot(httpd_req_t *req, char *post_data, int post_data_len) {
    app_config_t new_config;
    memset(&new_config, 0, sizeof(app_config_t));

    char param_buf[MAX_HOST_ADDR_LEN];
    char decoded_buf[MAX_HOST_ADDR_LEN];

    // Parse and URL-decode form data
    if (httpd_query_key_value(post_data, "device_name", param_buf, sizeof(param_buf)) == ESP_OK) {
        url_decode(decoded_buf, param_buf);
        strncpy(new_config.device_name, decoded_buf, sizeof(new_config.device_name) - 1);
        new_config.device_name[sizeof(new_config.device_name) - 1] = '\0';
    }
    if (httpd_query_key_value(post_data, "host_addr", param_buf, sizeof(param_buf)) == ESP_OK) {
        url_decode(decoded_buf, param_buf);
        strncpy(new_config.host_addr, decoded_buf, sizeof(new_config.host_addr) - 1);
        new_config.host_addr[sizeof(new_config.host_addr) - 1] = '\0';
    }
    if (httpd_query_key_value(post_data, "wifi_ssid", param_buf, sizeof(param_buf)) == ESP_OK) {
        url_decode(decoded_buf, param_buf);
        strncpy(new_config.wifi_ssid, decoded_buf, sizeof(new_config.wifi_ssid) - 1);
        new_config.wifi_ssid[sizeof(new_config.wifi_ssid) - 1] = '\0';
    }
    if (httpd_query_key_value(post_data, "wifi_password", param_buf, sizeof(param_buf)) == ESP_OK) {
        url_decode(decoded_buf, param_buf);
        strncpy(new_config.wifi_password, decoded_buf, sizeof(new_config.wifi_password) - 1);
        new_config.wifi_password[sizeof(new_config.wifi_password) - 1] = '\0';
    }

    // Server-side validation
    if (strlen(new_config.wifi_ssid) == 0 || strlen(new_config.device_name) == 0 || strlen(new_config.host_addr) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing required fields! WiFi SSID, Device Name, or Host Address cannot be empty.");
        return ESP_FAIL;
    }

    if (!is_valid_hostname(new_config.device_name)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid device name. Must be 1-31 characters, alphanumeric and hyphen only, cannot start/end with hyphen.");
        return ESP_FAIL;
    }

    new_config.provisioned = true;
    esp_err_t save_err = nvs_manager_save_config(&new_config);

    if (save_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save configuration to NVS: %s", esp_err_to_name(save_err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save configuration to NVS!");
        return ESP_FAIL;
    }

    const char* success_html = "<h1>Configuration Saved! Device rebooting...</h1>";
    httpd_resp_send(req, success_html, HTTPD_RESP_USE_STRLEN);

    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

    return ESP_OK;
}

// URI handler for /save (POST) in Provisioning Mode
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

    return _save_config_and_reboot(req, buf, ret);
}

// URI handler for /scan_wifi (GET) in Provisioning Mode
static esp_err_t scan_wifi_get_handler(httpd_req_t *req) {
    scanned_wifi_ap_t *ap_records = NULL;
    uint16_t ap_count = 0;

    // Trigger a scan and update the internal cache
    esp_err_t err = wifi_manager_scan_networks();

    if (err != ESP_OK) {
        // If a scan is already running or Wi-Fi is busy, fall back to returning the last cache
        // instead of failing the UI.
        ESP_LOGW(TAG, "Wi-Fi scan request did not start (%s); returning cached list if available.", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Wi-Fi scan completed; returning updated cache.");
    }

    // Now get the newly cached networks
    err = wifi_manager_get_cached_networks(&ap_records, &ap_count);

    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) { // Check for actual error, not just no found
        ESP_LOGE(TAG, "Failed to get cached Wi-Fi networks after scan: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get cached Wi-Fi networks");
        return ESP_FAIL;
    }

    cJSON *root = cJSON_CreateArray();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON array for /scan_wifi");
        if (ap_records) {
            free(ap_records);
        }
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal error");
        return ESP_FAIL;
    }

    for (int i = 0; i < ap_count; i++) {
        cJSON *ap = cJSON_CreateObject();
        if (ap == NULL) {
            ESP_LOGE(TAG, "Failed to create JSON object for AP record");
            cJSON_Delete(root);
            if (ap_records) {
                free(ap_records);
            }
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal error");
            return ESP_FAIL;
        }
        cJSON_AddStringToObject(ap, "ssid", ap_records[i].ssid);
        cJSON_AddNumberToObject(ap, "rssi", ap_records[i].rssi);
        // Convert auth mode to a readable string (optional, for better UI)
        const char *auth_mode_str;
        switch (ap_records[i].authmode) {
            case WIFI_AUTH_OPEN:        auth_mode_str = "OPEN"; break;
            case WIFI_AUTH_WEP:         auth_mode_str = "WEP"; break;
            case WIFI_AUTH_WPA_PSK:     auth_mode_str = "WPA_PSK"; break;
            case WIFI_AUTH_WPA2_PSK:    auth_mode_str = "WPA2_PSK"; break;
            case WIFI_AUTH_WPA_WPA2_PSK: auth_mode_str = "WPA_WPA2_PSK"; break;
            case WIFI_AUTH_WPA2_ENTERPRISE: auth_mode_str = "WPA2_ENTERPRISE"; break;
            case WIFI_AUTH_WPA3_PSK:    auth_mode_str = "WPA3_PSK"; break;
            case WIFI_AUTH_WPA2_WPA3_PSK: auth_mode_str = "WPA2_WPA3_PSK"; break;
            case WIFI_AUTH_WAPI_PSK:    auth_mode_str = "WAPI_PSK"; break;
            default:                    auth_mode_str = "UNKNOWN"; break;
        }
        cJSON_AddStringToObject(ap, "authmode", auth_mode_str);
        cJSON_AddItemToArray(root, ap);
    }

    const char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON for /scan_wifi");
        cJSON_Delete(root);
        if (ap_records) {
            free(ap_records);
        }
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal error");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));

    cJSON_free((void *)json_string);
    cJSON_Delete(root);
    if (ap_records) {
        free(ap_records); // Free the memory allocated by wifi_manager_get_cached_networks
    }
    return ESP_OK;
}

// URI handler for /edit (GET) in Normal Mode
static esp_err_t normal_edit_get_handler(httpd_req_t *req) {
    char *wifi_options = (char *)calloc(1, 2048);
    char *html_response = (char *)calloc(1, 8192);
    if (!wifi_options || !html_response) {
        free(wifi_options);
        free(html_response);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }

    build_cached_wifi_options(wifi_options, 2048);
    snprintf(html_response, 8192, PROV_HTML_FORM,
             "/update", // Form action for updating
             MAX_DEVICE_NAME_LEN - 1, g_app_config.device_name,
             MAX_HOST_ADDR_LEN - 1, g_app_config.host_addr,
             wifi_options,
             MAX_WIFI_SSID_LEN - 1, g_app_config.wifi_ssid,
             MAX_WIFI_PASS_LEN - 1, g_app_config.wifi_password);

    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    free(wifi_options);
    free(html_response);
    return ESP_OK;
}

// URI handler for /update (POST) in Normal Mode
static esp_err_t normal_update_post_handler(httpd_req_t *req) {
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

    return _save_config_and_reboot(req, buf, ret);
}

// --- Server Control ---

esp_err_t web_server_start(void) {
    if (server) {
        ESP_LOGW(TAG, "Server already running. Stopping and restarting it.");
        web_server_stop(); // Explicitly stop the existing server
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.stack_size = 8192;

    ESP_LOGI(TAG, "Starting server for mode %d on port: '%d'", g_app_mode, config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers for mode %d", g_app_mode);

        // If not provisioned, always serve provisioning UI regardless of current mode.
        if (!g_app_config.provisioned || g_app_mode == MODE_PROVISIONING) {
            httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = prov_root_get_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &root_uri);

            httpd_uri_t save_uri = { .uri = "/save", .method = HTTP_POST, .handler = prov_save_post_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &save_uri);

            httpd_uri_t scan_wifi_uri = { .uri = "/scan_wifi", .method = HTTP_GET, .handler = scan_wifi_get_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &scan_wifi_uri);

            httpd_uri_t get_cached_wifi_uri = { .uri = "/get_cached_wifi_list", .method = HTTP_GET, .handler = get_cached_wifi_list_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &get_cached_wifi_uri);
        } else if (g_app_mode == MODE_NORMAL) {
            httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = normal_root_get_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &root_uri);

            httpd_uri_t api_status_uri = { .uri = "/api/status", .method = HTTP_GET, .handler = normal_api_status_get_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &api_status_uri);

            httpd_uri_t edit_uri = { .uri = "/edit", .method = HTTP_GET, .handler = normal_edit_get_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &edit_uri);

            httpd_uri_t update_uri = { .uri = "/update", .method = HTTP_POST, .handler = normal_update_post_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &update_uri);

            httpd_uri_t scan_wifi_uri = { .uri = "/scan_wifi", .method = HTTP_GET, .handler = scan_wifi_get_handler, .user_ctx = NULL };
            httpd_register_uri_handler(server, &scan_wifi_uri);
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