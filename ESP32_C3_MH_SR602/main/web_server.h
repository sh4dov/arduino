#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"

// Start the web server. The behavior of the server (e.g., which endpoints
// are available) will depend on the application's current mode.
esp_err_t web_server_start(void);

// Stop the web server.
esp_err_t web_server_stop(void);

#endif // WEB_SERVER_H
