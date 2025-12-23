#include "status_led.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define STATUS_LED_GPIO 8

static const char *TAG = "status_led";
static void status_led_timer_callback(void* arg);

static esp_timer_handle_t g_status_led_timer;
static status_led_state_t g_current_state = STATUS_LED_NORMAL;
static bool g_led_on = false;

void status_led_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    
    // Start with LED off
    gpio_set_level(STATUS_LED_GPIO, 1);

    const esp_timer_create_args_t timer_args = {
            .callback = &status_led_timer_callback,
            .name = "status_led_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &g_status_led_timer));

    ESP_LOGI(TAG, "Status LED initialized on GPIO %d", STATUS_LED_GPIO);
}

void status_led_set_state(status_led_state_t state) {
    if (state == g_current_state) {
        return; // No change
    }

    g_current_state = state;
    esp_timer_stop(g_status_led_timer);
    g_led_on = false; // Reset blink state

    uint64_t period_us = 0;

    switch (g_current_state) {
        case STATUS_LED_PROVISIONING: // Fast blink
            ESP_LOGI(TAG, "Set state to PROVISIONING (fast blink)");
            period_us = 250 * 1000; // 250ms period
            break;
        case STATUS_LED_WIFI_CONNECTING: // Slow blink
            ESP_LOGI(TAG, "Set state to WIFI_CONNECTING (slow blink)");
            period_us = 1000 * 1000; // 1s period
            break;
        case STATUS_LED_NORMAL:
            ESP_LOGI(TAG, "Set state to NORMAL (solid off)");
            gpio_set_level(STATUS_LED_GPIO, 1); // Solid off
            return; // No timer needed
    }

    if (period_us > 0) {
        ESP_ERROR_CHECK(esp_timer_start_periodic(g_status_led_timer, period_us));
        // Immediately set the first state
        status_led_timer_callback(NULL);
    }
}

static void status_led_timer_callback(void* arg) {
    g_led_on = !g_led_on;
    // Active low: 0 = ON, 1 = OFF
    gpio_set_level(STATUS_LED_GPIO, g_led_on ? 0 : 1);
}