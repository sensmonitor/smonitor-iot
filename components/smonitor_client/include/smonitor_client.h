#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "smonitor_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SMONITOR_CLIENT_STATUS_STARTING = 0,
    SMONITOR_CLIENT_STATUS_READY,
    SMONITOR_CLIENT_STATUS_DEVICE_NOT_REGISTERED,
    SMONITOR_CLIENT_STATUS_DISCONNECTED,
} smonitor_client_status_t;

esp_err_t smonitor_client_start(const char *device_serial);
bool smonitor_client_wait_for_device_config(int timeout_ms);
smonitor_client_status_t smonitor_client_status(void);
esp_err_t smonitor_client_send_samples(const smonitor_i2c_sample_t *samples,
                                       size_t sample_count);
void smonitor_client_process(void);

#ifdef __cplusplus
}
#endif
