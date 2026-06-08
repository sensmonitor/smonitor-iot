#pragma once

#include <array>
#include <string>

#include "esp_err.h"

esp_err_t smonitor_device_serial(std::string &serial);
