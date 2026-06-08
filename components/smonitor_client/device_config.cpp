#include "device_config.hpp"

#include <cmath>

SensorType sensorTypeFromConfigValue(int value)
{
    switch (value) {
    case 1:
        return SensorType::Temperature;
    case 2:
        return SensorType::Humidity;
    case 3:
        return SensorType::Pressure;
    default:
        return SensorType::Unknown;
    }
}

double normalizeSampleValue(SensorType type, double value)
{
    switch (type) {
    case SensorType::Temperature:
    case SensorType::Humidity:
    case SensorType::Pressure:
        return std::round(value * 100.0) / 100.0;
    case SensorType::Unknown:
    default:
        return value;
    }
}
