#include "sensor_runtime.hpp"

#include "esp_check.h"
#include "esp_log.h"
#include "sdkconfig.h"

namespace {

constexpr const char *TAG = "smonitor_sensor";

smonitor_i2c_handle_t reader = nullptr;

} // namespace

esp_err_t smonitor_sensor_init(void)
{
    ESP_RETURN_ON_FALSE(reader == nullptr, ESP_ERR_INVALID_STATE, TAG,
                        "Sensor runtime is already initialized");

    smonitor_i2c_profile_config_t config{};
    config.port = CONFIG_SMONITOR_I2C_PORT;
    config.sda_pin = CONFIG_SMONITOR_I2C_SDA_PIN;
    config.scl_pin = CONFIG_SMONITOR_I2C_SCL_PIN;
    config.frequency_hz = CONFIG_SMONITOR_I2C_FREQUENCY_HZ;
#if CONFIG_SMONITOR_I2C_INTERNAL_PULLUPS
    config.internal_pullups = true;
#else
    config.internal_pullups = false;
#endif
    config.device_address = CONFIG_SMONITOR_I2C_DEVICE_ADDRESS;

    ESP_LOGI(TAG,
             "Initializing %s on I2C%d, SDA=%d, SCL=%d, address=0x%02X",
             smonitor_i2c_selected_sensor_name(),
             config.port,
             config.sda_pin,
             config.scl_pin,
             config.device_address);

    const esp_err_t result =
        smonitor_i2c_create_selected_profile(&config, &reader);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "%s initialized successfully",
                 smonitor_i2c_selected_sensor_name());
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
