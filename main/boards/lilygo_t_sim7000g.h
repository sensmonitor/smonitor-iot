#pragma once

#include "esp_err.h"
#include "smonitor_modem.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t smonitor_lilygo_t_sim7000g_power_init(void *context);
esp_err_t smonitor_lilygo_t_sim7000g_power_on(void *context);
smonitor_modem_config_t smonitor_lilygo_t_sim7000g_modem_config(void);

#ifdef __cplusplus
}
#endif
