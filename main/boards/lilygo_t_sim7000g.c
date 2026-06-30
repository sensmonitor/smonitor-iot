#include "boards/lilygo_t_sim7000g.h"

#include "boards/common/modem_gpio_power.h"
#include "sdkconfig.h"

static smonitor_modem_gpio_power_config_t modem_power_config(void)
{
    const smonitor_modem_profile_t *profile =
        smonitor_modem_configured_profile();
    const smonitor_modem_gpio_power_config_t config = {
        .pin = CONFIG_SMONITOR_BOARD_MODEM_PWRKEY_PIN,
        .pulse_ms = profile != NULL ? profile->pwrkey_pulse_ms : 1000,
        .startup_delay_ms =
            profile != NULL ? profile->startup_delay_ms : 2000,
        .active_level =
            profile != NULL ? profile->pwrkey_active_level : 1,
    };

    return config;
}

esp_err_t smonitor_lilygo_t_sim7000g_power_init(void *context)
{
    (void)context;
    const smonitor_modem_gpio_power_config_t config = modem_power_config();
    return smonitor_modem_gpio_power_init(&config);
}

esp_err_t smonitor_lilygo_t_sim7000g_power_on(void *context)
{
    (void)context;
    const smonitor_modem_gpio_power_config_t config = modem_power_config();
    return smonitor_modem_gpio_power_on(&config);
}

smonitor_modem_config_t smonitor_lilygo_t_sim7000g_modem_config(void)
{
    const smonitor_modem_config_t config = {
        .model = smonitor_modem_configured_model(),
        .uart = {
            .tx_pin = CONFIG_SMONITOR_BOARD_MODEM_UART_TX_PIN,
            .rx_pin = CONFIG_SMONITOR_BOARD_MODEM_UART_RX_PIN,
            .rts_pin = CONFIG_SMONITOR_BOARD_MODEM_UART_RTS_PIN,
            .cts_pin = CONFIG_SMONITOR_BOARD_MODEM_UART_CTS_PIN,
            .baud_rate = CONFIG_SMONITOR_BOARD_MODEM_UART_BAUD_RATE,
            .rx_buffer_size =
                CONFIG_SMONITOR_BOARD_MODEM_UART_RX_BUFFER_SIZE,
            .tx_buffer_size =
                CONFIG_SMONITOR_BOARD_MODEM_UART_TX_BUFFER_SIZE,
        },
        .power_init = smonitor_lilygo_t_sim7000g_power_init,
        .power_on = smonitor_lilygo_t_sim7000g_power_on,
        .power_context = NULL,
    };

    return config;
}
