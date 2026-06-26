#pragma once

#include <string>

#include <cJSON.h>

#include "device_config.hpp"
#include "smonitor_client.h"
#include "smonitor_i2c.h"

std::string formatJSON(const smonitor_i2c_sample_t *samples,
                       std::size_t sample_count,
                       const DeviceConfig &config,
                       const std::string &device_serial,
                       int battery_percent,
                       const smonitor_client_location_t &location);
cJSON *resolveConfigNode(cJSON *root, bool &should_delete);
bool handleDeviceConfigPayload(const std::string &payload, DeviceConfig &config);
bool handleDigitalIoConfigPayload(const std::string &payload, std::vector<DigitalIoConfig> &digital_ios);
