#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int i2c_port;
    int sda_pin;
    int scl_pin;
} smonitor_axp2101_config_t;

esp_err_t smonitor_axp2101_init_modem_power(
    const smonitor_axp2101_config_t *config);

#ifdef __cplusplus
}
#endif
