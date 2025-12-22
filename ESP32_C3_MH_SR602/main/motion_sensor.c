#include "motion_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "motion_sensor";
static gpio_num_t s_pir_gpio;
static motion_detection_callback_t s_motion_callback = NULL;
static QueueHandle_t gpio_evt_queue = NULL;
static motion_status_t s_current_status = MOTION_NONE; // Track current status

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void motion_sensor_task(void* arg) {
    uint32_t io_num;
    int last_sensor_level = gpio_get_level(s_pir_gpio); // Read initial state
    bool motion_active = (last_sensor_level == 1);
    s_current_status = motion_active ? MOTION_DETECTED : MOTION_NONE;

    


    while(1) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            int new_level = gpio_get_level(io_num);

            if (new_level == 1 && !motion_active) { // LOW to HIGH transition
                motion_active = true;
                s_current_status = MOTION_DETECTED;
                ESP_LOGI(TAG, "Motion detected (GPIO%d: HIGH)", io_num);
                if (s_motion_callback) {
                    s_motion_callback(MOTION_DETECTED);
                }
            } else if (new_level == 0 && motion_active) { // HIGH to LOW transition
                motion_active = false;
                s_current_status = MOTION_NONE;
                ESP_LOGI(TAG, "No motion detected (GPIO%d: LOW)", io_num);
                if (s_motion_callback) {
                    s_motion_callback(MOTION_NONE);
                }
            }
        }
    }
}

motion_status_t motion_sensor_get_status(void) {
    return s_current_status;
}

void motion_sensor_init(gpio_num_t pir_gpio, motion_detection_callback_t callback) {
    s_pir_gpio = pir_gpio;
    s_motion_callback = callback;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE, // Interrupt on both rising and falling edges
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << s_pir_gpio),
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(motion_sensor_task, "motion_sensor_task", 2048, NULL, 10, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(s_pir_gpio, gpio_isr_handler, (void*) s_pir_gpio);

    ESP_LOGI(TAG, "Motion sensor initialized on GPIO%d", s_pir_gpio);
}
