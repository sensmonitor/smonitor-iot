#include "app_controller.hpp"

#include <string>

#include "boards/board_profile.h"
#include "device_identity.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "sensor_runtime.hpp"
#include "smonitor_battery.h"
#include "smonitor_client.h"
#include "smonitor_modem.h"

namespace {

constexpr const char *TAG = "smonitor_iot";
constexpr std::size_t kMaxSensorSamples = 16;
constexpr int kFallbackBatteryPercent = 50;

smonitor_modem_network_t configuredNetwork()
{
#if CONFIG_SMONITOR_MODEM_NETWORK_LTE_M
    return SMONITOR_MODEM_NETWORK_LTE_M;
#elif CONFIG_SMONITOR_MODEM_NETWORK_NB_IOT
    return SMONITOR_MODEM_NETWORK_NB_IOT;
#elif CONFIG_SMONITOR_MODEM_NETWORK_GPRS
    return SMONITOR_MODEM_NETWORK_GPRS;
#else
    return SMONITOR_MODEM_NETWORK_AUTO;
#endif
}

void logSamples(const smonitor_i2c_sample_t *samples, std::size_t sample_count)
{
    for (std::size_t index = 0; index < sample_count; ++index) {
        ESP_LOGI(TAG, "%s/%s = %.2f %s valid=%d",
                 samples[index].device_id,
                 samples[index].output_id,
                 samples[index].value,
                 samples[index].unit,
                 samples[index].valid);
    }
}

void updateBatteryIfDue(int &battery_percent, TickType_t &next_battery_read_time)
{
#if CONFIG_SMONITOR_BATTERY_ENABLE
    if (CONFIG_SMONITOR_BATTERY_READ_INTERVAL_SECONDS == 0) {
        return;
    }

    const TickType_t now = xTaskGetTickCount();
    if ((int32_t)(now - next_battery_read_time) < 0) {
        return;
    }

    smonitor_battery_status_t battery = {};
    const esp_err_t result = smonitor_battery_read(&battery);
    if (result == ESP_OK && battery.valid) {
        battery_percent = battery.percent;
        ESP_LOGI(TAG, "Battery cache updated: %d%% (%d mV)",
                 battery.percent, battery.voltage_mv);
    } else {
        ESP_LOGW(TAG, "Battery read failed: %s; keeping %d%%",
                 esp_err_to_name(result), battery_percent);
    }

    next_battery_read_time =
        now + pdMS_TO_TICKS(CONFIG_SMONITOR_BATTERY_READ_INTERVAL_SECONDS *
                            1000);
#else
    (void)battery_percent;
    (void)next_battery_read_time;
#endif
}

smonitor_client_location_t readCachedLocation()
{
    smonitor_modem_location_t modem_location = {};
    const esp_err_t result = smonitor_modem_get_location(&modem_location);

    smonitor_client_location_t location = {
        .latitude = modem_location.latitude,
        .longitude = modem_location.longitude,
        .valid = result == ESP_OK && modem_location.valid,
    };

    if (location.valid) {
        ESP_LOGI(TAG, "Cached GPS location: latitude=%.6f, longitude=%.6f",
                 location.latitude, location.longitude);
    } else {
        ESP_LOGW(TAG, "Cached GPS location is not available; using 0,0");
    }

    return location;
}

} // namespace

void smonitor_app_run(void)
{
    std::string serial;
    ESP_ERROR_CHECK(smonitor_device_serial(serial));
    ESP_LOGI(TAG, "Device serial: %s", serial.c_str());

    ESP_ERROR_CHECK(smonitor_sensor_init());
#if CONFIG_SMONITOR_BATTERY_ENABLE
    ESP_ERROR_CHECK(smonitor_battery_init());
#endif

    if (CONFIG_SMONITOR_MODEM_APN[0] == '\0') {
        ESP_LOGE(TAG,
                 "APN is empty. Configure SensMonitor IoT > Mobile network APN.");
        return;
    }

    smonitor_modem_config_t modem_config = smonitor_board_modem_config();
    modem_config.apn = CONFIG_SMONITOR_MODEM_APN;
#if CONFIG_SMONITOR_MODEM_AUTH_PAP
    modem_config.username = CONFIG_SMONITOR_MODEM_USERNAME;
    modem_config.password = CONFIG_SMONITOR_MODEM_PASSWORD;
#else
    modem_config.username = nullptr;
    modem_config.password = nullptr;
#endif
    modem_config.network = configuredNetwork();

    ESP_ERROR_CHECK(smonitor_modem_init(&modem_config));
    ESP_ERROR_CHECK(
        smonitor_modem_connect(CONFIG_SMONITOR_MODEM_CONNECT_TIMEOUT_MS));
    const smonitor_client_location_t location = readCachedLocation();
    ESP_ERROR_CHECK(smonitor_client_start(serial.c_str()));

    if (!smonitor_client_wait_for_device_config(60000)) {
        if (smonitor_client_status() ==
            SMONITOR_CLIENT_STATUS_DEVICE_NOT_REGISTERED) {
            ESP_LOGE(TAG,
                     "DEVICE_NOT_REGISTERED: Register %s in the SensMonitor application.",
                     serial.c_str());
        } else {
            ESP_LOGW(TAG,
                     "Device config was not received during startup; retrying.");
        }
    }

    int battery_percent = kFallbackBatteryPercent;
    TickType_t next_battery_read_time = xTaskGetTickCount();
    TickType_t next_sample_time = xTaskGetTickCount();
    while (true) {
        updateBatteryIfDue(battery_percent, next_battery_read_time);

        smonitor_i2c_sample_t samples[kMaxSensorSamples] = {};
        std::size_t sample_count = 0;
        const esp_err_t read_result = smonitor_sensor_read(
            samples, kMaxSensorSamples, &sample_count);

        logSamples(samples, sample_count);
        if (read_result == ESP_OK) {
            const esp_err_t send_result =
                smonitor_client_send_samples(samples, sample_count,
                                             battery_percent, &location);
            if (send_result != ESP_OK &&
                send_result != ESP_ERR_INVALID_STATE) {
                ESP_LOGW(TAG, "Sample send failed: %s",
                         esp_err_to_name(send_result));
            }
        } else {
            ESP_LOGW(TAG, "Sensor read completed with %s",
                     esp_err_to_name(read_result));
        }

        smonitor_client_process();
        vTaskDelayUntil(&next_sample_time,
                        pdMS_TO_TICKS(CONFIG_SMONITOR_SAMPLE_INTERVAL_MS));
    }
}
