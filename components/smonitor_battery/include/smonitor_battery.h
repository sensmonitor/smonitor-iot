#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool valid;
    int percent;
    int voltage_mv;
    int adc_voltage_mv;
    int raw;
} smonitor_battery_status_t;

esp_err_t smonitor_battery_init(void);
esp_err_t smonitor_battery_read(smonitor_battery_status_t *status);

#ifdef __cplusplus
}
#endif
