#include "led_indicator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Hardware Configuration
#define LED_GPIO 8 // Onboard LED on ESP32-C3

static const char *TAG = "led_indicator";
static QueueHandle_t led_pattern_queue;

static void led_indicator_task(void *pvParameters) {
    led_pattern_t current_pattern = LED_SOLID_OFF;
    led_pattern_t new_pattern;
    uint8_t led_state = 0;
    TickType_t delay = portMAX_DELAY;

    while (1) {
        // Check for a new pattern without blocking
        if (xQueueReceive(led_pattern_queue, &new_pattern, 0) == pdPASS) {
            if (new_pattern != current_pattern) {
                ESP_LOGI(TAG, "Changing LED pattern to %d", new_pattern);
                current_pattern = new_pattern;
                // Reset state for the new pattern
                led_state = 0; 
            }
        }

        switch (current_pattern) {
            case LED_SOLID_ON:
                led_state = 1;
                delay = portMAX_DELAY; // No need to wake up until pattern changes
                break;
            case LED_SOLID_OFF:
                led_state = 0;
                delay = portMAX_DELAY;
                break;
            case LED_SLOW_BLINK:
                led_state = !led_state;
                delay = pdMS_TO_TICKS(1000); // 1s on, 1s off
                break;
            case LED_FAST_BLINK:
                led_state = !led_state;
                delay = pdMS_TO_TICKS(250); // 250ms on, 250ms off
                break;
        }

        gpio_set_level(LED_GPIO, led_state);
        vTaskDelay(delay);
    }
}

void led_indicator_init(void) {
    ESP_LOGI(TAG, "Initializing LED indicator");
    
    // Configure LED GPIO
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    // Create a queue to send pattern changes to the task
    led_pattern_queue = xQueueCreate(1, sizeof(led_pattern_t));
    
    // Set initial state
    led_indicator_set_pattern(LED_SOLID_OFF);

    xTaskCreate(led_indicator_task, "led_indicator_task", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "LED indicator task started");
}

void led_indicator_set_pattern(led_pattern_t pattern) {
    // Send the new pattern to the queue, overwriting if full
    xQueueOverwrite(led_pattern_queue, &pattern);
}
