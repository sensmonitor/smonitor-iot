#include "app_controller.hpp"

#include <string>

#include "device_identity.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "sensor_runtime.hpp"
#include "smonitor_client.h"
#include "smonitor_modem.h"

namespace {

constexpr const char *TAG = "smonitor_iot";
constexpr std::size_t kMaxSensorSamples = 16;

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

} // namespace

void smonitor_app_run(void)
{
    std::string serial;
    ESP_ERROR_CHECK(smonitor_device_serial(serial));
    ESP_LOGI(TAG, "Device serial: %s", serial.c_str());

    ESP_ERROR_CHECK(smonitor_sensor_init());

    if (CONFIG_SMONITOR_MODEM_APN[0] == '\0') {
        ESP_LOGE(TAG,
                 "APN is empty. Configure SensMonitor IoT > Mobile network APN.");
        return;
    }

    const smonitor_modem_config_t modem_config = {
        .apn = CONFIG_SMONITOR_MODEM_APN,
#if CONFIG_SMONITOR_MODEM_AUTH_PAP
        .username = CONFIG_SMONITOR_MODEM_USERNAME,
        .password = CONFIG_SMONITOR_MODEM_PASSWORD,
#else
        .username = nullptr,
        .password = nullptr,
#endif
        .network = configuredNetwork(),
    };

    ESP_ERROR_CHECK(smonitor_modem_init(&modem_config));
    ESP_ERROR_CHECK(
        smonitor_modem_connect(CONFIG_SMONITOR_MODEM_CONNECT_TIMEOUT_MS));
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

    TickType_t next_sample_time = xTaskGetTickCount();
    while (true) {
        smonitor_i2c_sample_t samples[kMaxSensorSamples] = {};
        std::size_t sample_count = 0;
        const esp_err_t read_result = smonitor_sensor_read(
            samples, kMaxSensorSamples, &sample_count);

        logSamples(samples, sample_count);
        if (read_result == ESP_OK) {
            const esp_err_t send_result =
                smonitor_client_send_samples(samples, sample_count);
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
