#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include "esp_err.h"

esp_err_t mdns_service_init(const char* device_name);
void mdns_service_stop(void);

#endif // MDNS_SERVICE_H
