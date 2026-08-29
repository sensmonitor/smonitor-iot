#include "boards/lilygo_t_sim7670_s3.h"

#include "boards/common/modem_gpio_power.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"

#define LILYGO_T_SIM7670_S3_MODEM_DTR_PIN 7
#define LILYGO_T_SIM7670_S3_POWER_SAVE_MODE_PIN 42

static smonitor_modem_gpio_power_config_t modem_power_config(void)
{
    const smonitor_modem_profile_t *profile =
        smonitor_modem_configured_profile();
    const smonitor_modem_gpio_power_config_t config = {
        .pin = CONFIG_SMONITOR_BOARD_MODEM_PWRKEY_PIN,
        .pulse_ms = profile != NULL ? profile->pwrkey_pulse_ms : 1000,
        .startup_delay_ms =
            profile != NULL ? profile->startup_delay_ms : 10000,
        .active_level =
            profile != NULL ? profile->pwrkey_active_level : 1,
    };

    return config;
}

esp_err_t smonitor_lilygo_t_sim7670_s3_power_init(void *context)
{
    (void)context;
    const smonitor_modem_gpio_power_config_t config = modem_power_config();
    ESP_RETURN_ON_ERROR(smonitor_modem_gpio_power_init(&config),
                        "lilygo_t_sim7670_s3",
                        "Failed to initialize modem PWRKEY GPIO");

    const gpio_config_t modem_control_gpio = {
        .pin_bit_mask = (1ULL << LILYGO_T_SIM7670_S3_MODEM_DTR_PIN) |
                        (1ULL << LILYGO_T_SIM7670_S3_POWER_SAVE_MODE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&modem_control_gpio),
                        "lilygo_t_sim7670_s3",
                        "Failed to configure modem DTR GPIO");
    ESP_RETURN_ON_ERROR(gpio_set_level(LILYGO_T_SIM7670_S3_MODEM_DTR_PIN, 0),
                        "lilygo_t_sim7670_s3",
                        "Failed to keep modem awake through DTR");
    return gpio_set_level(LILYGO_T_SIM7670_S3_POWER_SAVE_MODE_PIN, 1);
}

esp_err_t smonitor_lilygo_t_sim7670_s3_power_on(void *context)
{
    (void)context;
    const smonitor_modem_gpio_power_config_t config = modem_power_config();
    return smonitor_modem_gpio_power_on(&config);
}

smonitor_modem_config_t smonitor_lilygo_t_sim7670_s3_modem_config(void)
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
        .power_init = smonitor_lilygo_t_sim7670_s3_power_init,
        .power_on = smonitor_lilygo_t_sim7670_s3_power_on,
        .power_context = NULL,
    };

    return config;
}
