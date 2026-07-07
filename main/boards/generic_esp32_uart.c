#include "boards/generic_esp32_uart.h"

#include "boards/common/modem_gpio_power.h"
#include "sdkconfig.h"

#if CONFIG_SMONITOR_BOARD_MODEM_POWER_GPIO
static smonitor_modem_gpio_power_config_t power_config(void)
{
    const smonitor_modem_gpio_power_config_t config = {
        .pin = CONFIG_SMONITOR_BOARD_MODEM_PWRKEY_PIN,
        .pulse_ms = CONFIG_SMONITOR_BOARD_MODEM_PWRKEY_PULSE_MS,
        .startup_delay_ms = CONFIG_SMONITOR_BOARD_MODEM_STARTUP_DELAY_MS,
        .active_level = CONFIG_SMONITOR_BOARD_MODEM_PWRKEY_ACTIVE_LEVEL,
    };

    return config;
}

static esp_err_t generic_power_init(void *context)
{
    (void)context;
    const smonitor_modem_gpio_power_config_t config = power_config();
    return smonitor_modem_gpio_power_init(&config);
}

static esp_err_t generic_power_on(void *context)
{
    (void)context;
    const smonitor_modem_gpio_power_config_t config = power_config();
    return smonitor_modem_gpio_power_on(&config);
}
#else
static esp_err_t external_power_noop(void *context)
{
    (void)context;
    return ESP_OK;
}
#endif

smonitor_modem_config_t smonitor_generic_esp32_uart_modem_config(void)
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
#if CONFIG_SMONITOR_BOARD_MODEM_POWER_GPIO
        .power_init = generic_power_init,
        .power_on = generic_power_on,
#else
        .power_init = external_power_noop,
        .power_on = external_power_noop,
#endif
        .power_context = NULL,
    };

    return config;
}
