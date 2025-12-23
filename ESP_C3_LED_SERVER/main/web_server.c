#include "web_server.h"
#include "app_config.h"
#include "automation.h"
#include "wifi_manager.h" // For Wi-Fi scan APIs
#include "esp_system.h"   // For esp_restart

#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";

static httpd_handle_t server = NULL;

// Forward declarations
static esp_err_t main_page_handler(httpd_req_t *req);
static esp_err_t settings_page_handler(httpd_req_t *req);
static esp_err_t settings_post_handler(httpd_req_t *req);
static esp_err_t api_control_post_handler(httpd_req_t *req);
static esp_err_t api_motion_sensor_post_handler(httpd_req_t *req);
static esp_err_t api_status_get_handler(httpd_req_t *req);

static esp_err_t provisioning_page_handler(httpd_req_t *req);
static esp_err_t provisioning_post_handler(httpd_req_t *req);
static esp_err_t scan_wifi_handler(httpd_req_t *req);
static esp_err_t get_cached_wifi_list_handler(httpd_req_t *req);


// URI Definitions for Normal Mode
static const httpd_uri_t main_page_uri = { .uri = "/", .method = HTTP_GET, .handler = main_page_handler };
static const httpd_uri_t settings_page_uri = { .uri = "/settings", .method = HTTP_GET, .handler = settings_page_handler };
static const httpd_uri_t settings_post_uri = { .uri = "/settings", .method = HTTP_POST, .handler = settings_post_handler };
static const httpd_uri_t api_control_post_uri = { .uri = "/api/control", .method = HTTP_POST, .handler = api_control_post_handler };
static const httpd_uri_t api_motion_sensor_post_uri = { .uri = "/api/motionSensor", .method = HTTP_POST, .handler = api_motion_sensor_post_handler };
static const httpd_uri_t api_status_get_uri = { .uri = "/api/status", .method = HTTP_GET, .handler = api_status_get_handler };

// URI Definitions for Provisioning Mode
static const httpd_uri_t provisioning_page_uri = { .uri = "/", .method = HTTP_GET, .handler = provisioning_page_handler };
static const httpd_uri_t provisioning_post_uri = { .uri = "/save", .method = HTTP_POST, .handler = provisioning_post_handler };
static const httpd_uri_t scan_wifi_uri = { .uri = "/scan_wifi", .method = HTTP_GET, .handler = scan_wifi_handler };
static const httpd_uri_t get_cached_wifi_list_uri = { .uri = "/get_cached_wifi_list", .method = HTTP_GET, .handler = get_cached_wifi_list_handler };


esp_err_t web_server_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 10; // Increased to accommodate more handlers

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        app_config_t* app_config = app_config_get();
        if (app_config->provisioned) {
            ESP_LOGI(TAG, "Registering normal mode handlers");
            httpd_register_uri_handler(server, &main_page_uri);
            httpd_register_uri_handler(server, &settings_page_uri);
            httpd_register_uri_handler(server, &settings_post_uri);
            httpd_register_uri_handler(server, &api_control_post_uri);
            httpd_register_uri_handler(server, &api_motion_sensor_post_uri);
            httpd_register_uri_handler(server, &api_status_get_uri);
            // Scan and cached list also available in normal mode for /edit
            httpd_register_uri_handler(server, &scan_wifi_uri);
            httpd_register_uri_handler(server, &get_cached_wifi_list_uri);
        } else {
            ESP_LOGI(TAG, "Registering provisioning mode handlers");
            httpd_register_uri_handler(server, &provisioning_page_uri);
            httpd_register_uri_handler(server, &provisioning_post_uri);
            httpd_register_uri_handler(server, &scan_wifi_uri);
            httpd_register_uri_handler(server, &get_cached_wifi_list_uri);
        }
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error starting server!");
        return ESP_FAIL;
    }
}

esp_err_t web_server_stop(void) {
    if (server) {
        ESP_LOGI(TAG, "Stopping web server");
        return httpd_stop(server);
    }
    return ESP_OK;
}


// HTML content constants
static const char* HTML_HEAD_STYLE =
    "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>LED Controller</title><style>"
    "body{font-family:Arial,sans-serif;background-color:#f4f4f4;color:#333;margin:20px;}"
    ".container{background-color:#fff;padding:30px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);max-width:500px;margin:auto;}"
    "h1,h2{color:#0056b3;text-align:center;margin-bottom:25px;}"
    "label{display:block;margin-bottom:8px;font-weight:bold;}"
    "input[type='text'], input[type='password'], input[type='number'], select{width:calc(100%% - 22px);padding:10px;margin-bottom:15px;border:1px solid #ddd;border-radius:4px;font-size:16px;}"
    "input[type='submit'], button{background-color:#28a745;color:white;padding:12px 20px;border:none;border-radius:4px;cursor:pointer;font-size:16px;width:100%%;margin-top:10px;}"
    "input[type='submit']:hover, button:hover{background-color:#218838;}"
    ".switch{position:relative;display:inline-block;width:60px;height:34px;}"
    ".switch input{opacity:0;width:0;height:0;}"
    ".slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;transition:.4s;border-radius:34px;}"
    ".slider:before{position:absolute;content:'';height:26px;width:26px;left:4px;bottom:4px;background-color:white;transition:.4s;border-radius:50%;}"
    "input:checked+.slider{background-color:#2196F3;}"
    "input:checked+.slider:before{transform:translateX(26px);}"
    ".item, .setting{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;}"
    ".slider-container{display:flex;flex-direction:column;align-items:flex-start;width:100%%;}"
    ".slider-label{display:flex;justify-content:space-between;width:100%%;}"
    "</style></head><body><div class='container'>";

static const char* HTML_FOOT = "</div></body></html>";

static const char* SCRIPT_MAIN_PAGE = 
    "<script>"
    "function toggleItem(index, checked) {"
    "  var state = checked ? 'on' : 'off';"
    "  fetch('/api/control', {"
    "    method: 'POST',"
    "    headers: {'Content-Type': 'application/json'},"
    "    body: JSON.stringify({item_index: index, state: state})"
    "  });"
    "}"
    "</script>";

static const char* SCRIPT_SETTINGS_PAGE =
    "<script>"
    "function updateSlider(sliderId, outputId) {"
    "  document.getElementById(outputId).innerHTML = '&nbsp;(' + document.getElementById(sliderId).value + ')';"
    "}"
    "</script>";

static const char* SCRIPT_PROVISIONING_PAGE_PART1 =
    "<script>"
    "let scanIntervalId = null;"
    "const MAX_POLLING_ATTEMPTS = 10;"
    "let pollingAttempts = 0;"

    "function updateWifiList(data, currentSsid) {"
    "  const select = document.getElementById('ssid');"
    "  select.innerHTML = '<option value=\"\">--Select SSID--</option>';"
    "  if (data && data.length > 0) {"
    "    data.forEach(ap => {"
    "      const option = document.createElement('option');"
    "      option.value = ap.ssid;"
    "      option.innerText = ap.ssid + ' (RSSI: ' + ap.rssi + ')';"
    "      if (currentSsid && ap.ssid === currentSsid) {"
    "        option.selected = true;"
    "      }"
    "      select.appendChild(option);"
    "    });"
    "    document.getElementById('scanStatus').innerText = 'Scan Complete (' + data.length + ' networks found)'; "
    "  } else {"
    "    document.getElementById('scanStatus').innerText = 'Scan Complete (No networks found)'; "
    "  }"
    "}"

    "function fetchCachedWifiListAndSelect(currentSsid) {"
    "  fetch('/get_cached_wifi_list')"
    "    .then(response => response.json())"
    "    .then(data => {"
    "      if (data.error) {"
    "        console.error('Error fetching cached Wi-Fi list:', data.error);"
    "        document.getElementById('scanStatus').innerText = 'Error fetching networks';"
    "        return;"
    "      }"
    "      updateWifiList(data, currentSsid);"
    "    })"
    "    .catch(error => {"
    "      console.error('Error fetching cached Wi-Fi list:', error);"
    "      document.getElementById('scanStatus').innerText = 'Network error during fetch';"
    "    });"
    "}"

    "function pollForScanResults(currentSsid) {"
    "  pollingAttempts++;"
    "  if (pollingAttempts > MAX_POLLING_ATTEMPTS) {"
    "    clearInterval(scanIntervalId);"
    "    document.getElementById('scanStatus').innerText = 'Scan timed out. Try again.';"
    "    return;"
    "  }"

    "  fetch('/get_cached_wifi_list')"
    "    .then(response => response.json())"
    "    .then(data => {"
    "      if (data && data.length > 0) {"
    "        clearInterval(scanIntervalId);"
    "        updateWifiList(data, currentSsid);"
    "      } else {"
    "        document.getElementById('scanStatus').innerText = 'Scanning... (' + pollingAttempts + '/' + MAX_POLLING_ATTEMPTS + ')';"
    "      }"
    "    })"
    "    .catch(error => {"
    "      console.error('Error polling for Wi-Fi list:', error);"
    "      clearInterval(scanIntervalId);"
    "      document.getElementById('scanStatus').innerText = 'Scan polling failed';"
    "    });"
    "}"

    "function startScanAndPoll(currentSsid) {"
    "  if (scanIntervalId) clearInterval(scanIntervalId);"
    "  pollingAttempts = 0;"
    "  document.getElementById('scanStatus').innerText = 'Initiating scan...';"
    "  fetch('/scan_wifi')"
    "    .then(response => response.json())"
    "    .then(data => {"
    "      if (data.status === 'scan_started') {"
    "        document.getElementById('scanStatus').innerText = 'Scan started. Polling for results...';"
    "        scanIntervalId = setInterval(() => pollForScanResults(currentSsid), 2000); "
    "      } else {"
    "        document.getElementById('scanStatus').innerText = 'Failed to start scan';"
    "      }"
    "    })"
    "    .catch(error => {"
    "      console.error('Error starting scan:', error);"
    "      document.getElementById('scanStatus').innerText = 'Error starting scan request';"
    "    });"
    "}"
; // Note: No </script> here, it's in PART2

static const char* SCRIPT_PROVISIONING_PAGE_PART2 =
    "document.addEventListener('DOMContentLoaded', function() {"
    "  const scanButton = document.getElementById('scanWifiButton');"
    "  if (scanButton) {"
    "    scanButton.addEventListener('click', () => startScanAndPoll(null));"
    "  }"
    "  fetchCachedWifiListAndSelect(null);" // Initial fetch when DOM is ready for provisioning, no current SSID to select
    "});"
    "</script>";

static const char* SCRIPT_SETTINGS_WIFI_POPULATE_PART1 =
    "<script>"
    "let scanIntervalId = null;"
    "const MAX_POLLING_ATTEMPTS_SETTINGS = 10;"
    "let pollingAttemptsSettings = 0;"

    "function updateWifiListSettings(data, currentSsid) {"
    "  const select = document.getElementById('ssid');"
    "  select.innerHTML = '<option value=\"\">--Select SSID--</option>';"
    "  if (data && data.length > 0) {"
    "    data.forEach(ap => {"
    "      const option = document.createElement('option');"
    "      option.value = ap.ssid;"
    "      option.innerText = ap.ssid + ' (RSSI: ' + ap.rssi + ')';"
    "      if (currentSsid && ap.ssid === currentSsid) {"
    "        option.selected = true;"
    "      }"
    "      select.appendChild(option);"
    "    });"
    "    document.getElementById('scanStatus').innerText = 'Scan Complete (' + data.length + ' networks found)'; "
    "  } else {"
    "    document.getElementById('scanStatus').innerText = 'Scan Complete (No networks found)'; "
    "  }"
    "}"

    "function fetchCachedWifiListSettings(currentSsid) {"
    "  fetch('/get_cached_wifi_list')"
    "    .then(response => response.json())"
    "    .then(data => {"
    "      if (data.error) {"
    "        console.error('Error fetching cached Wi-Fi list:', data.error);"
    "        document.getElementById('scanStatus').innerText = 'Error fetching networks';"
    "        return;"
    "      }"
    "      updateWifiListSettings(data, currentSsid);"
    "    })"
    "    .catch(error => {"
    "      console.error('Error fetching cached Wi-Fi list:', error);"
    "      document.getElementById('scanStatus').innerText = 'Network error during fetch';"
    "    });"
    "}"

    "function pollForScanResultsSettings(currentSsid) {"
    "  pollingAttemptsSettings++;"
    "  if (pollingAttemptsSettings > MAX_POLLING_ATTEMPTS_SETTINGS) {"
    "    clearInterval(scanIntervalId);"
    "    document.getElementById('scanStatus').innerText = 'Scan timed out. Try again.';"
    "    return;"
    "  }"

    "  fetch('/get_cached_wifi_list')"
    "    .then(response => response.json())"
    "    .then(data => {"
    "      if (data && data.length > 0) {"
    "        clearInterval(scanIntervalId);"
    "        updateWifiListSettings(data, currentSsid);"
    "      } else {"
    "        document.getElementById('scanStatus').innerText = 'Scanning... (' + pollingAttemptsSettings + '/' + MAX_POLLING_ATTEMPTS_SETTINGS + ')';"
    "      }"
    "    })"
    "    .catch(error => {"
    "      console.error('Error polling for Wi-Fi list:', error);"
    "      clearInterval(scanIntervalId);"
    "      document.getElementById('scanStatus').innerText = 'Scan polling failed';"
    "    });"
    "}"

    "function startScanAndPollSettings(currentSsid) {"
    "  if (scanIntervalId) clearInterval(scanIntervalId);"
    "  pollingAttemptsSettings = 0;"
    "  document.getElementById('scanStatus').innerText = 'Initiating scan...';"
    "  fetch('/scan_wifi')"
    "    .then(response => response.json())"
    "    .then(data => {"
    "      if (data.status === 'scan_started') {"
    "        document.getElementById('scanStatus').innerText = 'Scan started. Polling for results...';"
    "        scanIntervalId = setInterval(() => pollForScanResultsSettings(currentSsid), 2000); "
    "      } else {"
    "        document.getElementById('scanStatus').innerText = 'Failed to start scan';"
    "      }"
    "    })"
    "    .catch(error => {"
    "      console.error('Error starting scan:', error);"
    "      document.getElementById('scanStatus').innerText = 'Error starting scan request';"
    "    });"
    "}"
; // Note: No </script> here, it's in PART2

static const char* SCRIPT_SETTINGS_WIFI_POPULATE_PART2 =
    "document.addEventListener('DOMContentLoaded', function() {"
    "  const currentSsid = window.CURRENT_WIFI_SSID;"
    "  const scanButton = document.getElementById('scanWifiButton');"
    "  if (scanButton) {"
    "    scanButton.addEventListener('click', () => startScanAndPollSettings(currentSsid));"
    "  }"
    "  startScanAndPollSettings(currentSsid);" // Initial scan and fetch when DOM is ready for settings page
    "});"
    "</script>";

// Generates the main page HTML
static esp_err_t main_page_handler(httpd_req_t *req) {
    char chunk[512];
    httpd_resp_set_type(req, "text/html");

    httpd_resp_send_chunk(req, HTML_HEAD_STYLE, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "<h1>LED Controller</h1>", HTTPD_RESP_USE_STRLEN);

    int num_items = app_config_get_num_items();
    const item_static_config_t* items = app_config_get_items();
    for (int i = 0; i < num_items; i++) {
        item_state_t state;
        if (!automation_get_item_state(i, &state)) {
            ESP_LOGE(TAG, "Failed to get state for item %d", i);
            state.is_on = false; // Fallback to off
        }
        snprintf(chunk, sizeof(chunk),
                 "<div class='item'><span>%s</span><label class='switch'>"
                 "<input type='checkbox' onchange='toggleItem(%d, this.checked)' %s>"
                 "<span class='slider'></span></label></div>",
                 items[i].name, i, state.is_on ? "checked" : "");
        httpd_resp_send_chunk(req, chunk, HTTPD_RESP_USE_STRLEN);
    }
    
    httpd_resp_send_chunk(req, "<br><a href='/settings'><button>Advanced Settings</button></a>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, SCRIPT_MAIN_PAGE, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, HTML_FOOT, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Generates the settings page HTML
static esp_err_t settings_page_handler(httpd_req_t *req) {
    char chunk[768];
    app_config_t* config = app_config_get();

    httpd_resp_send_chunk(req, HTML_HEAD_STYLE, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "<h2>Settings</h2><form method='post' action='/settings'>", HTTPD_RESP_USE_STRLEN);

    // Global Settings
    snprintf(chunk, sizeof(chunk),
             "<div class='setting'><label for='auto_timer'>Automation Timer (sec)</label><input type='number' id='auto_timer' name='auto_timer' value='%d'></div>"
             "<div class='setting'><label for='manual_timer'>Manual Override (hr)</label><input type='number' id='manual_timer' name='manual_timer' value='%d'></div>"
             "<div class='setting'><label for='fade_timer'>Fade Duration (sec)</label><input type='number' id='fade_timer' name='fade_timer' value='%d'></div>",
             (int)config->automation_timer_sec, (int)config->manual_override_timer_hr, (int)config->fade_duration_sec);
    httpd_resp_send_chunk(req, chunk, HTTPD_RESP_USE_STRLEN);

    // Per-Item Brightness
    httpd_resp_send_chunk(req, "<h2>Brightness</h2>", HTTPD_RESP_USE_STRLEN);
    int num_items = app_config_get_num_items();
    const item_static_config_t* items = app_config_get_items();
    for (int i = 0; i < num_items; i++) {
        snprintf(chunk, sizeof(chunk),
                 "<div class='item slider-container'>"
                 "<div class='slider-label'><span>%s</span><span id='brightness_val_%d'>&nbsp;(%d)</span></div>"
                 "<input type='range' min='0' max='100' value='%d' name='brightness_%d' id='brightness_slider_%d' oninput='updateSlider(\"brightness_slider_%d\", \"brightness_val_%d\")'>"
                 "</div>",
                 items[i].name, i, config->item_brightness[i], config->item_brightness[i], i, i, i, i);
        httpd_resp_send_chunk(req, chunk, HTTPD_RESP_USE_STRLEN);
    }

    // Network Settings
    httpd_resp_send_chunk(req, "<h2>Network Settings</h2>", HTTPD_RESP_USE_STRLEN);

    char current_ssid[33] = {0}; // Max SSID len + null terminator
    char current_password[65] = {0}; // Max password len + null terminator

    // Get current Wi-Fi settings for pre-filling
    wifi_manager_get_sta_config(current_ssid, sizeof(current_ssid), current_password, sizeof(current_password));

    snprintf(chunk, sizeof(chunk),
             "<label for='server_name'>Device Name:</label>"
             "<input type='text' id='server_name' name='server_name' value='%s' required>"
             "<label for='ssid'>Wi-Fi SSID:</label>"
             "<select id='ssid' name='ssid'><option value=''>--Select SSID--</option></select>" // Populated by JS
             "<button type='button' id='scanWifiButton'>Scan Wi-Fi</button>"
             "<p id='scanStatus'></p>"
             "<label for='password'>Wi-Fi Password:</label>"
             "<input type='password' id='password' name='password' value='%s'>" // Pre-fill with current password (if stored/available)                          
             "<script>window.CURRENT_WIFI_SSID = '%s';</script>", // Inject current SSID for JS
             config->server_name, current_password, current_ssid);
    httpd_resp_send_chunk(req, chunk, HTTPD_RESP_USE_STRLEN);

    // Footer with buttons
    httpd_resp_send_chunk(req, "<br><button type='submit'>Save & Reboot</button> <a href='/'><button type='button'>Back</button></a>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "</form>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, SCRIPT_SETTINGS_PAGE, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, SCRIPT_SETTINGS_WIFI_POPULATE_PART1, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, SCRIPT_SETTINGS_WIFI_POPULATE_PART2, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, HTML_FOOT, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Handler for provisioning page
static esp_err_t provisioning_page_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");

    app_config_t* config = app_config_get();
    char current_device_name[sizeof(config->server_name)];
    strncpy(current_device_name, config->server_name, sizeof(current_device_name));
    current_device_name[sizeof(current_device_name) - 1] = '\0';
    if (!config->provisioned && strcmp(current_device_name, "led-server") == 0) {
        // If not provisioned and still default name, clear it for user input
        current_device_name[0] = '\0';
    }


    char html_form[2048];
    snprintf(html_form, sizeof(html_form),
             "<h1>Device Setup</h1>"
             "<form action='/save' method='post'>"
             "<label for='server_name'>Device Name:</label>"
             "<input type='text' id='server_name' name='server_name' value='%s' required>"
             "<label for='ssid'>Wi-Fi SSID:</label>"
             "<select id='ssid' name='ssid' required><option value=''>--Select SSID--</option></select>" // Populated by JS
             "<label for='password'>Wi-Fi Password:</label>"
             "<input type='password' id='password' name='password'>"
             "<button type='button' id='scanWifiButton'>Scan Wi-Fi</button>"
             "<p id='scanStatus'></p>"
             "<input type='submit' value='Save Configuration'>"
             "</form>",
             current_device_name);

    httpd_resp_send_chunk(req, HTML_HEAD_STYLE, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, html_form, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, SCRIPT_PROVISIONING_PAGE_PART1, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, SCRIPT_PROVISIONING_PAGE_PART2, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, HTML_FOOT, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


// Handler for saving settings
static esp_err_t settings_post_handler(httpd_req_t *req) {
    char buf[512]; // Increased buffer size to accommodate more fields
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        ESP_LOGE(TAG, "POST content too long: %d bytes", remaining);
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Content too large");
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0'; // Null-terminate the received data
    
    app_config_t* config = app_config_get();
    char param_val[65]; // Increased param_val size for password

    // Flag to indicate if network settings changed, requiring a reboot
    bool network_settings_changed = false;

    // Process Global Settings
    if (httpd_query_key_value(buf, "auto_timer", param_val, sizeof(param_val)) == ESP_OK) config->automation_timer_sec = atoi(param_val);
    if (httpd_query_key_value(buf, "manual_timer", param_val, sizeof(param_val)) == ESP_OK) config->manual_override_timer_hr = atoi(param_val);
    if (httpd_query_key_value(buf, "fade_timer", param_val, sizeof(param_val)) == ESP_OK) config->fade_duration_sec = atoi(param_val);

    // Process Per-Item Brightness
    int num_items = app_config_get_num_items();
    for (int i = 0; i < num_items; i++) {
        char param_key[32];
        snprintf(param_key, sizeof(param_key), "brightness_%d", i);
        if (httpd_query_key_value(buf, param_key, param_val, sizeof(param_val)) == ESP_OK) config->item_brightness[i] = atoi(param_val);
    }

    // Process Network Settings
    char server_name[sizeof(config->server_name)];
    char ssid[sizeof(config->wifi_ssid)];
    char password[sizeof(config->wifi_password)];

    if (httpd_query_key_value(buf, "server_name", server_name, sizeof(server_name)) == ESP_OK) {
        if (strncmp(config->server_name, server_name, sizeof(config->server_name)) != 0) {
            strncpy(config->server_name, server_name, sizeof(config->server_name) - 1);
            config->server_name[sizeof(config->server_name) - 1] = '\0';
            network_settings_changed = true;
        }
    }

    // SSID and password are optional if not changed
    if (httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) == ESP_OK) {
        if (strncmp(config->wifi_ssid, ssid, sizeof(config->wifi_ssid)) != 0) {
            strncpy(config->wifi_ssid, ssid, sizeof(config->wifi_ssid) - 1);
            config->wifi_ssid[sizeof(config->wifi_ssid) - 1] = '\0';
            network_settings_changed = true;
        }
    }
    if (httpd_query_key_value(buf, "password", password, sizeof(password)) == ESP_OK) {
        if (strncmp(config->wifi_password, password, sizeof(config->wifi_password)) != 0) {
            strncpy(config->wifi_password, password, sizeof(config->wifi_password) - 1);
            config->wifi_password[sizeof(config->wifi_password) - 1] = '\0';
            network_settings_changed = true;
        }
    }
    
    app_config_save();
    ESP_LOGI(TAG, "Configuration saved.");

    if (network_settings_changed) {
        ESP_LOGI(TAG, "Network settings changed. Rebooting in 2 seconds...");
        httpd_resp_sendstr(req, "Settings saved. Network settings changed, device is rebooting...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    } else {
        httpd_resp_sendstr(req, "Settings saved.");
    }
    return ESP_OK;
}

// Handler for saving provisioning data
static esp_err_t provisioning_post_handler(httpd_req_t *req) {
    char buf[256]; // Max content length for POST body
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        ESP_LOGE(TAG, "POST content too long: %d bytes", remaining);
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Content too large");
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0'; // Null-terminate the received data
    
    app_config_t* config = app_config_get();
    char server_name[sizeof(config->server_name)];
    char ssid[sizeof(config->wifi_ssid)];
    char password[sizeof(config->wifi_password)];

    // Using httpd_query_key_value to extract form data
    if (httpd_query_key_value(buf, "server_name", server_name, sizeof(server_name)) != ESP_OK ||
        httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing required fields");
        return ESP_FAIL;
    }
    
    // Password can be empty, so no check for ESP_OK
    httpd_query_key_value(buf, "password", password, sizeof(password));

    strncpy(config->server_name, server_name, sizeof(config->server_name) - 1);
    config->server_name[sizeof(config->server_name) - 1] = '\0'; // Ensure null termination
    strncpy(config->wifi_ssid, ssid, sizeof(config->wifi_ssid) - 1);
    config->wifi_ssid[sizeof(config->wifi_ssid) - 1] = '\0'; // Ensure null termination
    strncpy(config->wifi_password, password, sizeof(config->wifi_password) - 1);
    config->wifi_password[sizeof(config->wifi_password) - 1] = '\0'; // Ensure null termination
    config->provisioned = true;
    
    app_config_save();
    ESP_LOGI(TAG, "Provisioning data saved. Rebooting in 2 seconds...");

    httpd_resp_sendstr(req, "Credentials saved. Device is rebooting...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

    return ESP_OK;
}

// Handler for /scan_wifi (GET)
static esp_err_t scan_wifi_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Received /scan_wifi request. Initiating scan.");
    wifi_manager_scan_networks_async(); // Trigger an async scan

    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddStringToObject(response_json, "status", "scan_started");
    char *json_string = cJSON_PrintUnformatted(response_json);
    httpd_resp_sendstr(req, json_string);
    cJSON_Delete(response_json);
    free(json_string);
    
    return ESP_OK;
}

// Handler for /get_cached_wifi_list (GET)
static esp_err_t get_cached_wifi_list_handler(httpd_req_t *req) {
    scanned_wifi_ap_t *ap_records = NULL;
    uint16_t ap_count = 0;
    esp_err_t err = wifi_manager_get_cached_networks(&ap_records, &ap_count);

    httpd_resp_set_type(req, "application/json");

    if (err != ESP_OK) {
        cJSON *error_json = cJSON_CreateObject();
        cJSON_AddStringToObject(error_json, "error", esp_err_to_name(err));
        char *json_string = cJSON_PrintUnformatted(error_json);
        httpd_resp_sendstr(req, json_string);
        cJSON_Delete(error_json);
        free(json_string);
        return ESP_FAIL;
    }

    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < ap_count; i++) {
        cJSON *ap_item = cJSON_CreateObject();
        cJSON_AddStringToObject(ap_item, "ssid", ap_records[i].ssid);
        cJSON_AddNumberToObject(ap_item, "rssi", ap_records[i].rssi);
        // Authentication mode can be added if needed, mapping values to strings
        cJSON_AddItemToArray(root, ap_item);
    }
    free(ap_records); // Free memory allocated by wifi_manager_get_cached_networks

    char *json_string = cJSON_PrintUnformatted(root);
    ESP_LOGI(TAG, "Sending Wi-Fi list: %s", json_string); // DEBUG LOG
    httpd_resp_sendstr(req, json_string);
    cJSON_Delete(root);
    free(json_string);

    return ESP_OK;
}


#include "app_config.h"
#include "esp_log.h"
#include "cJSON.h"
#include "automation.h"

// ... (other includes and definitions)

static esp_err_t api_control_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request: Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *item_json = cJSON_GetObjectItem(root, "item");
    cJSON *item_index_json = cJSON_GetObjectItem(root, "item_index");
    cJSON *state_json = cJSON_GetObjectItem(root, "state");
    cJSON *brightness_json = cJSON_GetObjectItem(root, "brightness");

    int item_index = -1;

    if (cJSON_IsString(item_json)) {
        item_index = app_config_get_item_index_by_name(item_json->valuestring);
    } else if (cJSON_IsNumber(item_index_json)) {
        item_index = item_index_json->valueint;
    }

    if (item_index == -1 || item_index >= app_config_get_num_items() || !cJSON_IsString(state_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request: Missing or invalid 'item'/'item_index' or 'state'");
        return ESP_FAIL;
    }

    const char *state_str = state_json->valuestring;
    bool on = (strcmp(state_str, "on") == 0);
    
    // Get the configured brightness for the item
    app_config_t *app_config = app_config_get();
    uint8_t default_brightness = app_config->item_brightness[item_index];

    uint8_t brightness = cJSON_IsNumber(brightness_json) ? brightness_json->valueint : default_brightness;

    ESP_LOGI(TAG, "Manual control: item %d, state %s, brightness %d", item_index, state_str, brightness);
    automation_set_item_manual(item_index, on, brightness);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

// Handler for /api/motionSensor (POST)
static esp_err_t api_motion_sensor_post_handler(httpd_req_t *req) {
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';

    cJSON *root = cJSON_Parse(content);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *sensor_name_json = cJSON_GetObjectItem(root, "sensor");
    cJSON *state_json = cJSON_GetObjectItem(root, "state");

    if (cJSON_IsString(sensor_name_json) && (cJSON_IsNumber(state_json) || cJSON_IsBool(state_json))) {
        const char* sensor_name = sensor_name_json->valuestring;
        uint8_t state = cJSON_IsBool(state_json) ? (uint8_t)cJSON_IsTrue(state_json) : (uint8_t)state_json->valueint;

        automation_handle_motion_event(sensor_name, state);
        httpd_resp_sendstr(req, "OK");
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    cJSON_Delete(root);
    return ESP_OK;
}

// Handler for /api/status (GET) (stub)
static esp_err_t api_status_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving /api/status");
    httpd_resp_set_type(req, "application/json");
    // This should be a JSON response with all item states

    cJSON *root = cJSON_CreateObject();
    cJSON *items_array = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "items", items_array);

    int num_items = app_config_get_num_items();
    const item_static_config_t* static_items = app_config_get_items();

    for (int i = 0; i < num_items; i++) {
        cJSON *item_obj = cJSON_CreateObject();
        item_state_t state;
        if (!automation_get_item_state(i, &state)) {
            ESP_LOGE(TAG, "Failed to get state for item %d for API status", i);
            state.is_on = false;
            state.brightness = 0;
            state.control_mode = MODE_AUTOMATIC;
        }

        cJSON_AddStringToObject(item_obj, "name", static_items[i].name);
        cJSON_AddBoolToObject(item_obj, "is_on", state.is_on);
        cJSON_AddNumberToObject(item_obj, "brightness", state.brightness);
        cJSON_AddStringToObject(item_obj, "control_mode", state.control_mode == MODE_AUTOMATIC ? "automatic" : "manual");
        
        cJSON_AddItemToArray(items_array, item_obj);
    }

    char *json_string = cJSON_Print(root);
    if (json_string == NULL) {
        ESP_LOGE(TAG, "Failed to print cJSON object");
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to generate JSON");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, json_string);
    free(json_string);
    cJSON_Delete(root);
    
    return ESP_OK;
}