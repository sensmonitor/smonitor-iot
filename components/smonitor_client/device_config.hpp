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
    int id = 0;
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
    int plc_rule = 0; // PLC rule that changed the output, reported until the next ack
};

struct PlcRuleConfig {
    int id = 0;
    int sensor_id = 0;
    std::string operator_slug;
    double input_value = 0.0;
    int digital_output = 0;
    bool output_value = false;
    bool enabled = false;
    std::string updated_at;
};

struct DeviceConfig {
    std::string serial;
    std::vector<SensorConfig> sensors;
    std::vector<DigitalIoConfig> digital_ios;
    std::vector<PlcRuleConfig> plc_rules;
    bool has_plc_rules = false;
};

SensorType sensorTypeFromConfigValue(int value);
const char *sensorSampleType(SensorType type);
double normalizeSampleValue(SensorType type, double value);
