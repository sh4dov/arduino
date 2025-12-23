#include "led_control.h"
#include "app_config.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "led_control";

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_RESOLUTION         LEDC_TIMER_13_BIT // 8192 resolution
#define LEDC_FREQUENCY_HZ       (5000)

// Store the channel for each LED strip of each item
static ledc_channel_t g_led_channels[MAX_ITEMS][MAX_LED_STRIPS_PER_ITEM];

void led_control_init(void) {
    // 1. Configure LEDC Timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_RESOLUTION,
        .freq_hz          = LEDC_FREQUENCY_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Configure LEDC Channels
    const item_static_config_t* items = app_config_get_items();
    int num_items = app_config_get_num_items();
    int channel_num = 0;

    for (int i = 0; i < num_items; i++) {
        for (int j = 0; j < MAX_LED_STRIPS_PER_ITEM; j++) {
            int gpio = items[i].led_gpios[j];
            if (gpio >= 0 && channel_num < LEDC_CHANNEL_MAX) {
                g_led_channels[i][j] = (ledc_channel_t)channel_num;
                
                ledc_channel_config_t ledc_channel = {
                    .speed_mode     = LEDC_MODE,
                    .channel        = g_led_channels[i][j],
                    .timer_sel      = LEDC_TIMER,
                    .intr_type      = LEDC_INTR_DISABLE,
                    .gpio_num       = gpio,
                    .duty           = 0, // Set duty to 0%
                    .hpoint         = 0
                };
                ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
                channel_num++;
            } else {
                 g_led_channels[i][j] = -1; // Mark as unused
            }
        }
    }
    
    // Install fade function
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
    ESP_LOGI(TAG, "Initialized %d LED channels", channel_num);
}

static uint32_t brightness_to_duty(uint8_t brightness_percent) {
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    // Linear conversion from 0-100 to 0-8191
    return (uint32_t)(brightness_percent * 8191) / 100;
}

void led_control_set_brightness(int item_index, uint8_t brightness_percent) {
    if (item_index < 0 || item_index >= app_config_get_num_items()) {
        return;
    }

    uint32_t duty = brightness_to_duty(brightness_percent);

    for (int i = 0; i < MAX_LED_STRIPS_PER_ITEM; i++) {
        ledc_channel_t channel = g_led_channels[item_index][i];
        if (channel != -1) {
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, channel, duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, channel));
        }
    }
}

void led_control_fade_to(int item_index, uint8_t target_brightness_percent, uint32_t duration_ms) {
    if (item_index < 0 || item_index >= app_config_get_num_items()) {
        return;
    }
    
    uint32_t target_duty = brightness_to_duty(target_brightness_percent);

    for (int i = 0; i < MAX_LED_STRIPS_PER_ITEM; i++) {
        ledc_channel_t channel = g_led_channels[item_index][i];
        if (channel != -1) {
            ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_MODE, channel, target_duty, duration_ms));
            ESP_ERROR_CHECK(ledc_fade_start(LEDC_MODE, channel, LEDC_FADE_NO_WAIT));
        }
    }
}

void led_control_handle_boot_sequence(led_boot_sequence_step_t step) {
    int num_items = app_config_get_num_items();
    static bool blink_state = false;
    app_config_t* config = app_config_get();

    switch (step) {
        case BOOT_SEQ_INITIAL_BLINK:
            // This case should be called periodically to create a blink effect
            blink_state = !blink_state;
            for (int i = 0; i < num_items; i++) {
                led_control_set_brightness(i, blink_state ? 20 : 0);
            }
            break;

        case BOOT_SEQ_START_WIFI_CONNECT:
            // This case should be called periodically to create a blink effect
            blink_state = !blink_state;
            for (int i = 0; i < num_items; i++) {
                led_control_set_brightness(i, blink_state ? 20 : 0);
            }
            break;

        case BOOT_SEQ_WIFI_CONNECTED:
            ESP_LOGI(TAG, "Boot sequence: Wi-Fi connected, LEDs solid 20%%");
            for (int i = 0; i < num_items; i++) {
                // Set instantly to 20%
                led_control_set_brightness(i, 20);
            }
            break;

        case BOOT_SEQ_NORMAL_OPERATION:
            ESP_LOGI(TAG, "Boot sequence: Fading out to enter normal operation");
            for (int i = 0; i < num_items; i++) {
                led_control_fade_to(i, 0, config->fade_duration_sec * 1000);
            }
            break;
    }
}