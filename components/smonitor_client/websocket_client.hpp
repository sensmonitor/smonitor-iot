#pragma once

#include <string>

#include "device_config.hpp"
#include "smonitor_client.h"
#include "smonitor_i2c.h"

void initWSClient(const std::string &device_serial);
void checkWSConnection();
void sendData(std::string data);
void sendSample(const smonitor_i2c_sample_t *samples, std::size_t sample_count,
                int battery_percent,
                const smonitor_client_location_t &location);
bool waitForDeviceConfig(int timeout_ms);
bool hasDeviceConfig();
bool isRegistrationRequired();
DeviceConfig getDeviceConfig();
