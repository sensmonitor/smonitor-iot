#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t pin;
    uint32_t pulse_ms;
    uint32_t startup_delay_ms;
    uint8_t active_level;
} smonitor_modem_gpio_power_config_t;

esp_err_t smonitor_modem_gpio_power_init(
    const smonitor_modem_gpio_power_config_t *config);
esp_err_t smonitor_modem_gpio_power_on(
    const smonitor_modem_gpio_power_config_t *config);

#ifdef __cplusplus
}
#endif
