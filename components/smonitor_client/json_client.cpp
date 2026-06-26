#include "json_client.hpp"

#include <cstdlib>

namespace {

const smonitor_i2c_sample_t *findSample(
    const smonitor_i2c_sample_t *samples,
    std::size_t sample_count,
    SensorType type)
{
    const char *expected = nullptr;
    switch (type) {
    case SensorType::Temperature:
        expected = "temperature";
        break;
    case SensorType::Humidity:
        expected = "humidity";
        break;
    case SensorType::Pressure:
        expected = "pressure";
        break;
    default:
        return nullptr;
    }
    for (std::size_t index = 0; index < sample_count; ++index) {
        if (samples[index].valid && std::string(samples[index].type) == expected) {
            return &samples[index];
        }
    }
    return nullptr;
}

void addSensorValue(cJSON *sensors, const SensorConfig &sensor_config,
                    const smonitor_i2c_sample_t *samples,
                    std::size_t sample_count)
{
    const smonitor_i2c_sample_t *sample =
        findSample(samples, sample_count, sensor_config.type);
    if (sample == nullptr) {
        return;
    }
    cJSON *sensor = cJSON_CreateObject();
    cJSON_AddItemToArray(sensors, sensor);
    cJSON_AddStringToObject(sensor, "name", sensor_config.serial.c_str());
    cJSON_AddNumberToObject(
        sensor, "value",
        normalizeSampleValue(sensor_config.type, sample->value));
}

SensorType sensorTypeFromSlug(const char *slug)
{
    if (slug == nullptr) {
        return SensorType::Unknown;
    }

    std::string value = slug;
    if (value == "temperature") {
        return SensorType::Temperature;
    }
    if (value == "humidity") {
        return SensorType::Humidity;
    }
    if (value == "pressure") {
        return SensorType::Pressure;
    }

    return SensorType::Unknown;
}

SensorType parseSensorType(cJSON *type_item)
{
    if (cJSON_IsNumber(type_item)) {
        return sensorTypeFromConfigValue(type_item->valueint);
    }

    if (!cJSON_IsObject(type_item)) {
        return SensorType::Unknown;
    }

    cJSON *id_item = cJSON_GetObjectItemCaseSensitive(type_item, "id");
    if (cJSON_IsNumber(id_item)) {
        SensorType sensor_type = sensorTypeFromConfigValue(id_item->valueint);
        if (sensor_type != SensorType::Unknown) {
            return sensor_type;
        }
    }

    cJSON *slug_item = cJSON_GetObjectItemCaseSensitive(type_item, "slug");
    if (cJSON_IsString(slug_item) && slug_item->valuestring != nullptr) {
        SensorType sensor_type = sensorTypeFromSlug(slug_item->valuestring);
        if (sensor_type != SensorType::Unknown) {
            return sensor_type;
        }
    }

    cJSON *name_item = cJSON_GetObjectItemCaseSensitive(type_item, "name");
    if (cJSON_IsString(name_item) && name_item->valuestring != nullptr) {
        return sensorTypeFromSlug(name_item->valuestring);
    }

    return SensorType::Unknown;
}

bool parseSensorConfig(cJSON *sensor_node, SensorConfig &sensor_config)
{
    if (!cJSON_IsObject(sensor_node)) {
        return false;
    }

    cJSON *serial_item = cJSON_GetObjectItemCaseSensitive(sensor_node, "serial");
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(sensor_node, "type");
    if (!cJSON_IsString(serial_item) || serial_item->valuestring == nullptr) {
        return false;
    }

    SensorType sensor_type = parseSensorType(type_item);
    if (sensor_type == SensorType::Unknown) {
        return false;
    }

    sensor_config.serial = serial_item->valuestring;
    sensor_config.type = sensor_type;
    return true;
}

bool parseDigitalIoConfig(cJSON *digital_io_node, DigitalIoConfig &digital_io_config)
{
    if (!cJSON_IsObject(digital_io_node)) {
        return false;
    }

    cJSON *id_item = cJSON_GetObjectItemCaseSensitive(digital_io_node, "id");
    cJSON *pin_item = cJSON_GetObjectItemCaseSensitive(digital_io_node, "pin");
    cJSON *value_item = cJSON_GetObjectItemCaseSensitive(digital_io_node, "value");
    cJSON *desired_value_item = cJSON_GetObjectItemCaseSensitive(digital_io_node, "desired_value");
    cJSON *config_pending_item = cJSON_GetObjectItemCaseSensitive(digital_io_node, "config_pending");
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(digital_io_node, "type");
    int type_value = 0;

    if (cJSON_IsNumber(type_item)) {
        type_value = type_item->valueint;
    } else if (cJSON_IsObject(type_item)) {
        cJSON *type_id_item = cJSON_GetObjectItemCaseSensitive(type_item, "id");
        if (cJSON_IsNumber(type_id_item)) {
            type_value = type_id_item->valueint;
        }
    }

    if (!cJSON_IsNumber(pin_item) || !cJSON_IsBool(value_item) || type_value == 0) {
        return false;
    }

    digital_io_config.id = cJSON_IsNumber(id_item) ? id_item->valueint : 0;
    digital_io_config.pin = pin_item->valueint;
    digital_io_config.value = cJSON_IsTrue(value_item);
    digital_io_config.desired_value = cJSON_IsBool(desired_value_item)
        ? cJSON_IsTrue(desired_value_item)
        : digital_io_config.value;
    digital_io_config.config_pending = cJSON_IsBool(config_pending_item) && cJSON_IsTrue(config_pending_item);
    digital_io_config.type = type_value;
    return true;
}

bool parseDeviceConfigNode(cJSON *config_node, DeviceConfig &config)
{
    if (!cJSON_IsObject(config_node)) {
        return false;
    }

    DeviceConfig parsed_config;

    cJSON *serial_item = cJSON_GetObjectItemCaseSensitive(config_node, "serial");
    if (cJSON_IsString(serial_item) && serial_item->valuestring != nullptr) {
        parsed_config.serial = serial_item->valuestring;
    }

    cJSON *sensors_item = cJSON_GetObjectItemCaseSensitive(config_node, "sensors");
    if (cJSON_IsArray(sensors_item)) {
        cJSON *sensor_node = nullptr;
        cJSON_ArrayForEach(sensor_node, sensors_item) {
            SensorConfig sensor_config;
            if (parseSensorConfig(sensor_node, sensor_config)) {
                parsed_config.sensors.push_back(sensor_config);
            }
        }
    }

    cJSON *digital_ios_item = cJSON_GetObjectItemCaseSensitive(config_node, "digital_ios");
    if (cJSON_IsArray(digital_ios_item)) {
        cJSON *digital_io_node = nullptr;
        cJSON_ArrayForEach(digital_io_node, digital_ios_item) {
            DigitalIoConfig digital_io_config{};
            if (parseDigitalIoConfig(digital_io_node, digital_io_config)) {
                parsed_config.digital_ios.push_back(digital_io_config);
            }
        }
    }

    if (parsed_config.sensors.empty()) {
        return false;
    }

    config = parsed_config;
    return true;
}

cJSON *findDeviceNode(cJSON *node)
{
    if (!cJSON_IsObject(node)) {
        return nullptr;
    }

    if (cJSON_GetObjectItemCaseSensitive(node, "sensors") != nullptr) {
        return node;
    }

    cJSON *device = cJSON_GetObjectItemCaseSensitive(node, "device");
    if (cJSON_IsObject(device) && cJSON_GetObjectItemCaseSensitive(device, "sensors") != nullptr) {
        return device;
    }

    return nullptr;
}

} // namespace

cJSON *resolveConfigNode(cJSON *root, bool &should_delete)
{
    should_delete = false;
    if (root == nullptr) {
        return nullptr;
    }

    cJSON *device_node = findDeviceNode(root);
    if (device_node != nullptr) {
        return device_node;
    }

    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (cJSON_IsObject(data)) {
        device_node = findDeviceNode(data);
        if (device_node != nullptr) {
            return device_node;
        }

        cJSON *config = cJSON_GetObjectItemCaseSensitive(data, "config");
        if (cJSON_IsObject(config)) {
            device_node = findDeviceNode(config);
            return device_node != nullptr ? device_node : config;
        }
    }

    if (cJSON_IsString(data) && data->valuestring != nullptr) {
        cJSON *nested = cJSON_Parse(data->valuestring);
        if (nested == nullptr) {
            return nullptr;
        }

        should_delete = true;
        device_node = findDeviceNode(nested);
        if (device_node != nullptr) {
            cJSON *detached = cJSON_Duplicate(device_node, 1);
            cJSON_Delete(nested);
            return detached;
        }

        cJSON *config = cJSON_GetObjectItemCaseSensitive(nested, "config");
        if (cJSON_IsObject(config)) {
            cJSON *config_device = findDeviceNode(config);
            cJSON *detached = cJSON_Duplicate(config_device != nullptr ? config_device : config, 1);
            cJSON_Delete(nested);
            return detached;
        }

        return nested;
    }

    return nullptr;
}

bool handleDeviceConfigPayload(const std::string &payload, DeviceConfig &config)
{
    cJSON *root = cJSON_ParseWithLength(payload.c_str(), payload.size());
    if (root == nullptr) {
        return false;
    }

    bool should_delete_config_node = false;
    cJSON *config_node = resolveConfigNode(root, should_delete_config_node);
    if (config_node == nullptr) {
        cJSON_Delete(root);
        return false;
    }

    bool parsed = parseDeviceConfigNode(config_node, config);

    if (should_delete_config_node) {
        cJSON_Delete(config_node);
    }
    cJSON_Delete(root);
    return parsed;
}

bool parseDigitalIoArray(cJSON *digital_ios_item, std::vector<DigitalIoConfig> &digital_ios)
{
    if (!cJSON_IsArray(digital_ios_item)) {
        return false;
    }

    std::vector<DigitalIoConfig> parsed_digital_ios;
    cJSON *digital_io_node = nullptr;
    cJSON_ArrayForEach(digital_io_node, digital_ios_item) {
        DigitalIoConfig digital_io_config{};
        if (parseDigitalIoConfig(digital_io_node, digital_io_config)) {
            parsed_digital_ios.push_back(digital_io_config);
        }
    }

    if (parsed_digital_ios.empty()) {
        return false;
    }

    digital_ios = parsed_digital_ios;
    return true;
}

cJSON *findDigitalIosArray(cJSON *node)
{
    if (!cJSON_IsObject(node)) {
        return nullptr;
    }

    cJSON *digital_ios = cJSON_GetObjectItemCaseSensitive(node, "digital_ios");
    if (cJSON_IsArray(digital_ios)) {
        return digital_ios;
    }

    cJSON *device = cJSON_GetObjectItemCaseSensitive(node, "device");
    if (cJSON_IsObject(device)) {
        digital_ios = cJSON_GetObjectItemCaseSensitive(device, "digital_ios");
        if (cJSON_IsArray(digital_ios)) {
            return digital_ios;
        }
    }

    cJSON *config = cJSON_GetObjectItemCaseSensitive(node, "config");
    if (cJSON_IsObject(config)) {
        return findDigitalIosArray(config);
    }

    return nullptr;
}

bool handleDigitalIoConfigPayload(const std::string &payload, std::vector<DigitalIoConfig> &digital_ios)
{
    cJSON *root = cJSON_ParseWithLength(payload.c_str(), payload.size());

    if (root == nullptr) {
        return false;
    }

    bool parsed = parseDigitalIoArray(findDigitalIosArray(root), digital_ios);

    if (!parsed) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
        if (cJSON_IsObject(data)) {
            parsed = parseDigitalIoArray(findDigitalIosArray(data), digital_ios);
        } else if (cJSON_IsString(data) && data->valuestring != nullptr) {
            cJSON *nested = cJSON_Parse(data->valuestring);
            if (nested != nullptr) {
                parsed = parseDigitalIoArray(findDigitalIosArray(nested), digital_ios);
                cJSON_Delete(nested);
            }
        }
    }

    cJSON_Delete(root);
    return parsed;
}

std::string formatJSON(const smonitor_i2c_sample_t *samples,
                       std::size_t sample_count,
                       const DeviceConfig &config,
                       const std::string &device_serial,
                       int battery_percent,
                       const smonitor_client_location_t &location_value)
{
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "deviceId", device_serial.c_str());
    cJSON_AddNumberToObject(data, "battery", battery_percent);

    cJSON *sensors = cJSON_CreateArray();
    cJSON_AddItemToObject(data, "sensors", sensors);

    for (const SensorConfig &sensor_config : config.sensors) {
        addSensorValue(sensors, sensor_config, samples, sample_count);
    }

    cJSON *digital_ios = cJSON_CreateArray();
    cJSON_AddItemToObject(data, "digital_ios", digital_ios);

    for (const DigitalIoConfig &digital_io_config : config.digital_ios) {
        cJSON *digital_io = cJSON_CreateObject();
        cJSON_AddItemToArray(digital_ios, digital_io);

        if (digital_io_config.id > 0) {
            cJSON_AddNumberToObject(digital_io, "id", digital_io_config.id);
        }

        cJSON_AddNumberToObject(digital_io, "pin", digital_io_config.pin);
        cJSON_AddBoolToObject(digital_io, "value", digital_io_config.value);
    }

    cJSON *location = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "location", location);
    const double latitude =
        location_value.valid ? location_value.latitude : 0.0;
    const double longitude =
        location_value.valid ? location_value.longitude : 0.0;
    cJSON_AddNumberToObject(location, "latitude", latitude);
    cJSON_AddNumberToObject(location, "longitude", longitude);

    char *raw_json = cJSON_PrintUnformatted(data);
    std::string json = raw_json != nullptr ? raw_json : "{}";

    cJSON_Delete(data);
    if (raw_json != nullptr) {
        std::free(raw_json);
    }

    return json;
}
