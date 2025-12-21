#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include "esp_err.h"

// Initialize the mDNS service
// device_name: The name to advertise the device as (e.g., "my-sensor").
esp_err_t mdns_service_init(const char* device_name);

// Stop the mDNS service
void mdns_service_stop(void);

#endif // MDNS_SERVICE_H
