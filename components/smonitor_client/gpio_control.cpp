#include "gpio_control.hpp"

#include "driver/gpio.h"
#include "esp_log.h"

namespace {
constexpr int kDigitalIoInputType = 1;
constexpr int kDigitalIoOutputType = 2;
const char *TAG = "gpio_control";

bool validGpioPin(int pin)
{
    return pin >= 0 && pin < GPIO_NUM_MAX;
}
}

void applyDigitalIoConfig(DeviceConfig &config)
{
    uint64_t output_pin_mask = 0;
    uint64_t input_pin_mask = 0;

    for (const DigitalIoConfig &digital_io : config.digital_ios) {
        if (!validGpioPin(digital_io.pin)) {
            ESP_LOGW(TAG, "Skipping invalid digital I/O pin: %d", digital_io.pin);
            continue;
        }

        if (digital_io.type == kDigitalIoOutputType) {
            output_pin_mask |= (1ULL << digital_io.pin);
        } else if (digital_io.type == kDigitalIoInputType) {
            input_pin_mask |= (1ULL << digital_io.pin);
        }
    }

    if (output_pin_mask != 0) {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = output_pin_mask;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
    }

    if (input_pin_mask != 0) {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = input_pin_mask;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
    }

    for (DigitalIoConfig &digital_io : config.digital_ios) {
        if (!validGpioPin(digital_io.pin)) {
            continue;
        }

        if (digital_io.type == kDigitalIoOutputType) {
            const int level = digital_io.desired_value ? 1 : 0;
            gpio_set_level(static_cast<gpio_num_t>(digital_io.pin), level);
            digital_io.value = digital_io.desired_value;
            ESP_LOGI(TAG, "Applied digital output pin=%d value=%d", digital_io.pin, level);
        } else if (digital_io.type == kDigitalIoInputType) {
            digital_io.value = gpio_get_level(static_cast<gpio_num_t>(digital_io.pin)) != 0;
        }
    }
}
