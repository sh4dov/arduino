#ifndef HOST_CLIENT_H
#define HOST_CLIENT_H

#include "app_state.h"

/**
 * @brief Sends a motion state update to the configured host server.
 *
 * This function is non-blocking. It creates a new task to handle the
 * HTTP POST request, including the retry mechanism.
 *
 * @param status The new motion status (MOTION_DETECTED or MOTION_NONE).
 */
void host_client_send_motion_update(motion_status_t status);

#endif // HOST_CLIENT_H
