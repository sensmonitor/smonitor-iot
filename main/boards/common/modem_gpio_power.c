#include "boards/common/modem_gpio_power.h"

#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "modem_gpio_power";

esp_err_t smonitor_modem_gpio_power_init(
    const smonitor_modem_gpio_power_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Power configuration is required");
    ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(config->pin),
                        ESP_ERR_INVALID_ARG, TAG,
                        "PWRKEY pin must be a valid output GPIO");
    ESP_RETURN_ON_FALSE(config->active_level <= 1, ESP_ERR_INVALID_ARG, TAG,
                        "Active level must be 0 or 1");

    const gpio_config_t gpio = {
        .pin_bit_mask = 1ULL << config->pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&gpio), TAG,
                        "Failed to configure PWRKEY GPIO");
    return gpio_set_level(config->pin, !config->active_level);
}

esp_err_t smonitor_modem_gpio_power_on(
    const smonitor_modem_gpio_power_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Power configuration is required");

    ESP_RETURN_ON_ERROR(
        gpio_set_level(config->pin, config->active_level), TAG,
        "Failed to assert PWRKEY");
    vTaskDelay(pdMS_TO_TICKS(config->pulse_ms));
    ESP_RETURN_ON_ERROR(
        gpio_set_level(config->pin, !config->active_level), TAG,
        "Failed to release PWRKEY");
    vTaskDelay(pdMS_TO_TICKS(config->startup_delay_ms));
    return ESP_OK;
}
