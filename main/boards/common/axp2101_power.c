#include "boards/common/axp2101_power.h"

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "axp2101_power";

#define AXP2101_ADDR 0x34
#define AXP2101_CHIP_ID_REG 0x03
#define AXP2101_CHIP_ID 0x4A
#define AXP2101_DC_ONOFF_REG 0x80
#define AXP2101_DC3_VOLTAGE_REG 0x84
#define AXP2101_LDO_ONOFF_REG 0x90
#define AXP2101_BLDO2_VOLTAGE_REG 0x97
#define AXP2101_TS_PIN_CTRL_REG 0x50

#define AXP2101_DC3_ENABLE_BIT (1U << 2)
#define AXP2101_BLDO2_ENABLE_BIT (1U << 5)

static i2c_master_bus_handle_t pmu_bus;
static i2c_master_dev_handle_t pmu_device;

static esp_err_t axp2101_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(pmu_device, &reg, 1, value, 1, 100);
}

static esp_err_t axp2101_write_reg(uint8_t reg, uint8_t value)
{
    const uint8_t data[] = {reg, value};
    return i2c_master_transmit(pmu_device, data, sizeof(data), 100);
}

static esp_err_t axp2101_update_reg(uint8_t reg, uint8_t clear_mask,
                                    uint8_t set_mask)
{
    uint8_t value = 0;
    ESP_RETURN_ON_ERROR(axp2101_read_reg(reg, &value), TAG,
                        "Failed to read AXP2101 register 0x%02x", reg);
    value = (value & ~clear_mask) | set_mask;
    return axp2101_write_reg(reg, value);
}

static uint8_t axp2101_dc3_voltage_value_3000mv(void)
{
    return 88 + ((3000 - 1600) / 100);
}

static uint8_t axp2101_bldo2_voltage_value_3300mv(void)
{
    return (3300 - 500) / 100;
}

esp_err_t smonitor_axp2101_init_modem_power(
    const smonitor_axp2101_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "AXP2101 configuration is required");

    if (pmu_device != NULL) {
        ESP_LOGI(TAG, "AXP2101 modem power was already initialized");
        return ESP_OK;
    }

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = config->i2c_port,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
            .allow_pd = 0,
        },
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &pmu_bus), TAG,
                        "Failed to create AXP2101 I2C bus");

    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AXP2101_ADDR,
        .scl_speed_hz = 100000,
        .scl_wait_us = 0,
        .flags = {
            .disable_ack_check = 0,
        },
    };

    ESP_RETURN_ON_ERROR(
        i2c_master_bus_add_device(pmu_bus, &device_config, &pmu_device), TAG,
        "Failed to add AXP2101 I2C device");

    uint8_t chip_id = 0;
    ESP_RETURN_ON_ERROR(axp2101_read_reg(AXP2101_CHIP_ID_REG, &chip_id), TAG,
                        "Failed to read AXP2101 chip ID");
    ESP_RETURN_ON_FALSE(chip_id == AXP2101_CHIP_ID, ESP_ERR_NOT_FOUND, TAG,
                        "Unexpected PMU chip ID 0x%02x", chip_id);

    ESP_RETURN_ON_ERROR(
        axp2101_write_reg(AXP2101_DC3_VOLTAGE_REG,
                          axp2101_dc3_voltage_value_3000mv()),
        TAG, "Failed to set AXP2101 DC3 voltage");
    ESP_RETURN_ON_ERROR(
        axp2101_update_reg(AXP2101_DC_ONOFF_REG, 0, AXP2101_DC3_ENABLE_BIT),
        TAG, "Failed to enable AXP2101 DC3 modem power");

    ESP_RETURN_ON_ERROR(
        axp2101_write_reg(AXP2101_BLDO2_VOLTAGE_REG,
                          axp2101_bldo2_voltage_value_3300mv()),
        TAG, "Failed to set AXP2101 BLDO2 voltage");
    ESP_RETURN_ON_ERROR(
        axp2101_update_reg(AXP2101_LDO_ONOFF_REG, 0,
                           AXP2101_BLDO2_ENABLE_BIT),
        TAG, "Failed to enable AXP2101 BLDO2 GNSS antenna power");

    ESP_RETURN_ON_ERROR(axp2101_write_reg(AXP2101_TS_PIN_CTRL_REG, 0x00), TAG,
                        "Failed to disable AXP2101 TS pin measurement");

    ESP_LOGI(TAG, "AXP2101 modem power enabled: DC3=3000mV, BLDO2=3300mV");
    vTaskDelay(pdMS_TO_TICKS(200));
    return ESP_OK;
}
