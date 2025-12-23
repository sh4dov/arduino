idf_component_register(SRCS "main.c"
                         "app_config.c"
                         "status_led.c"
                         "wifi_manager.c"
                         "mdns_service.c"
                         "led_control.c"
                         "automation.c"
                         "web_server.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                    nvs_flash
                    esp_wifi
                    esp_event
                    esp_netif
                    esp_http_server
                    esp_driver_gpio
                    espressif__mdns
                    esp_system
                    freertos
                    driver
                    esp_timer
                    json)
