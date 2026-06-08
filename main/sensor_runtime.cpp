#include "sensor_runtime.hpp"

#include <cstdio>

#include "esp_check.h"
#include "esp_log.h"
#include "sdkconfig.h"

namespace {

constexpr const char *TAG = "smonitor_sensor";
constexpr size_t kProfileCapacity = 768;

smonitor_i2c_handle_t reader = nullptr;

const char *configuredDriver()
{
#if CONFIG_SMONITOR_I2C_SENSOR_BME280
    return "bosch_bme280";
#else
#error Unsupported SensMonitor I2C sensor selection
#endif
}

const char *configuredSensorName()
{
#if CONFIG_SMONITOR_I2C_SENSOR_BME280
    return "BME280";
#else
#error Unsupported SensMonitor I2C sensor selection
#endif
}

} // namespace

esp_err_t smonitor_sensor_init(void)
{
    ESP_RETURN_ON_FALSE(reader == nullptr, ESP_ERR_INVALID_STATE, TAG,
                        "Sensor runtime is already initialized");

    char profile[kProfileCapacity];
    const int length = snprintf(
        profile,
        sizeof(profile),
        "{\"schema_version\":1,"
        "\"bus\":{\"port\":%d,\"sda_pin\":%d,\"scl_pin\":%d,"
        "\"frequency_hz\":%d,\"internal_pullups\":%s},"
        "\"devices\":[{\"id\":\"environment\",\"address\":%d,"
        "\"decoder\":{\"type\":\"native\",\"name\":\"%s\","
        "\"outputs\":["
        "{\"id\":\"temperature\",\"type\":\"temperature\",\"unit\":\"celsius\"},"
        "{\"id\":\"pressure\",\"type\":\"pressure\",\"unit\":\"hpa\"},"
        "{\"id\":\"humidity\",\"type\":\"humidity\",\"unit\":\"percent_rh\"}"
        "]}}]}",
        CONFIG_SMONITOR_I2C_PORT,
        CONFIG_SMONITOR_I2C_SDA_PIN,
        CONFIG_SMONITOR_I2C_SCL_PIN,
        CONFIG_SMONITOR_I2C_FREQUENCY_HZ,
        CONFIG_SMONITOR_I2C_INTERNAL_PULLUPS ? "true" : "false",
        CONFIG_SMONITOR_I2C_DEVICE_ADDRESS,
        configuredDriver());

    ESP_RETURN_ON_FALSE(length > 0 &&
                            static_cast<size_t>(length) < sizeof(profile),
                        ESP_ERR_INVALID_SIZE, TAG,
                        "Generated sensor profile is too large");

    ESP_LOGI(TAG,
             "Initializing %s on I2C%d, SDA=%d, SCL=%d, address=0x%02X",
             configuredSensorName(),
             CONFIG_SMONITOR_I2C_PORT,
             CONFIG_SMONITOR_I2C_SDA_PIN,
             CONFIG_SMONITOR_I2C_SCL_PIN,
             CONFIG_SMONITOR_I2C_DEVICE_ADDRESS);

    const esp_err_t result = smonitor_i2c_create_from_json(profile, &reader);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "%s initialized successfully", configuredSensorName());
    }
    return result;
}

esp_err_t smonitor_sensor_read(smonitor_i2c_sample_t *samples,
                               size_t capacity,
                               size_t *out_count)
{
    ESP_RETURN_ON_FALSE(reader != nullptr, ESP_ERR_INVALID_STATE, TAG,
                        "Sensor runtime is not initialized");
    return smonitor_i2c_read_all(reader, samples, capacity, out_count);
}

void smonitor_sensor_deinit(void)
{
    smonitor_i2c_destroy(&reader);
}
