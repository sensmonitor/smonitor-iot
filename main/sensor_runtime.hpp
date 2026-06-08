#pragma once

#include <cstddef>

#include "esp_err.h"
#include "smonitor_i2c.h"

esp_err_t smonitor_sensor_init(void);
esp_err_t smonitor_sensor_read(smonitor_i2c_sample_t *samples,
                               size_t capacity,
                               size_t *out_count);
void smonitor_sensor_deinit(void);
