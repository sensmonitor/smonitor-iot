#pragma once

#include "device_config.hpp"

void applyDigitalIoConfig(DeviceConfig &config);
void setDigitalOutput(int pin, bool value);
