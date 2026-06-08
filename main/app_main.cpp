#include <string>

#include "esp_log.h"
#include "smonitor_modem.h"
#include "device_identity.hpp"
#include "sdkconfig.h"

namespace {

constexpr const char *TAG = "smonitor_iot";

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

} // namespace

extern "C" void app_main(void)
{
    std::string serial;
    ESP_ERROR_CHECK(smonitor_device_serial(serial));
    ESP_LOGI(TAG, "Device serial: %s", serial.c_str());

    if (CONFIG_SMONITOR_MODEM_APN[0] == '\0') {
        ESP_LOGE(TAG, "APN is empty. Configure SensMonitor IoT > Mobile network APN.");
        return;
    }

    const smonitor_modem_config_t modem_config = {
        .apn = CONFIG_SMONITOR_MODEM_APN,
        .username = CONFIG_SMONITOR_MODEM_USERNAME,
        .password = CONFIG_SMONITOR_MODEM_PASSWORD,
        .network = configuredNetwork(),
    };

    ESP_ERROR_CHECK(smonitor_modem_init(&modem_config));
    ESP_LOGW(TAG, "Initial repository scaffold is ready; PPP and SensMonitor client migration follow.");
}
