#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"

/**
 * @brief Starts the HTTP web server and registers appropriate URI handlers
 *        based on the device's provisioning status.
 *
 * @return ESP_OK on success, error otherwise.
 */
esp_err_t web_server_start(void);

/**
 * @brief Stops the HTTP web server.
 *
 * @return ESP_OK on success, error otherwise.
 */
esp_err_t web_server_stop(void);

#endif // WEB_SERVER_H
