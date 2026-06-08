#pragma once

#include <cstddef>
#include <string>
#include <vector>

enum class SensorType : int {
    Unknown = 0,
    Temperature = 1,
    Humidity = 2,
    Pressure = 3,
};

struct SensorConfig {
    std::string serial;
    SensorType type;
};

struct DigitalIoConfig {
    int id;
    int pin;
    bool value;
    bool desired_value;
    bool config_pending;
    int type;
};

struct DeviceConfig {
    std::string serial;
    std::vector<SensorConfig> sensors;
    std::vector<DigitalIoConfig> digital_ios;
};

SensorType sensorTypeFromConfigValue(int value);
double normalizeSampleValue(SensorType type, double value);
