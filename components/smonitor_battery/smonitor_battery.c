#include "smonitor_battery.h"

#include <string.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "smonitor_battery";

#if CONFIG_SMONITOR_BATTERY_ENABLE

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static adc_unit_t adc_unit;
static adc_channel_t adc_channel;
static bool initialized;
static bool calibrated;

static int clamp_percent(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

static int battery_percent_from_mv(int voltage_mv)
{
    static const struct {
        int mv;
        int percent;
    } curve[] = {
        {4200, 100},
        {4100, 90},
        {4000, 80},
        {3900, 65},
        {3800, 50},
        {3700, 35},
        {3600, 20},
        {3500, 10},
        {3300, 0},
    };

    if (voltage_mv >= curve[0].mv) {
        return curve[0].percent;
    }

    const size_t curve_count = sizeof(curve) / sizeof(curve[0]);
    for (size_t index = 1; index < curve_count; ++index) {
        if (voltage_mv >= curve[index].mv) {
            const int high_mv = curve[index - 1].mv;
            const int low_mv = curve[index].mv;
            const int high_percent = curve[index - 1].percent;
            const int low_percent = curve[index].percent;

            return low_percent +
                   ((voltage_mv - low_mv) * (high_percent - low_percent)) /
                       (high_mv - low_mv);
        }
    }

    return curve[curve_count - 1].percent;
}

esp_err_t smonitor_battery_init(void)
{
    if (initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(
        adc_oneshot_io_to_channel(CONFIG_SMONITOR_BATTERY_ADC_GPIO,
                                  &adc_unit, &adc_channel),
        TAG, "Battery GPIO cannot be mapped to an ADC channel");

    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = adc_unit,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_config, &adc_handle),
                        TAG, "Failed to initialize ADC unit");

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_RETURN_ON_ERROR(
        adc_oneshot_config_channel(adc_handle, adc_channel, &channel_config),
        TAG, "Failed to configure battery ADC channel");

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = adc_unit,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    calibrated =
        adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle) ==
        ESP_OK;
#endif

    ESP_RETURN_ON_FALSE(calibrated, ESP_ERR_NOT_SUPPORTED, TAG,
                        "Battery ADC calibration is not available");

    initialized = true;
    ESP_LOGI(TAG, "Battery ADC initialized on GPIO%d (unit=%d channel=%d)",
             CONFIG_SMONITOR_BATTERY_ADC_GPIO, adc_unit, adc_channel);
    return ESP_OK;
}

esp_err_t smonitor_battery_read(smonitor_battery_status_t *status)
{
    ESP_RETURN_ON_FALSE(status != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Battery status output is required");
    ESP_RETURN_ON_ERROR(smonitor_battery_init(), TAG,
                        "Battery ADC init failed");

    memset(status, 0, sizeof(*status));

    int raw_sum = 0;
    for (int index = 0; index < CONFIG_SMONITOR_BATTERY_ADC_SAMPLE_COUNT;
         ++index) {
        int raw = 0;
        ESP_RETURN_ON_ERROR(adc_oneshot_read(adc_handle, adc_channel, &raw),
                            TAG, "Battery ADC read failed");
        raw_sum += raw;
    }

    const int raw_average =
        raw_sum / CONFIG_SMONITOR_BATTERY_ADC_SAMPLE_COUNT;
    int adc_voltage_mv = 0;
    ESP_RETURN_ON_ERROR(
        adc_cali_raw_to_voltage(cali_handle, raw_average, &adc_voltage_mv),
        TAG, "Battery ADC voltage conversion failed");

    const int battery_voltage_mv =
        (adc_voltage_mv * CONFIG_SMONITOR_BATTERY_DIVIDER_RATIO_MILLI + 500) /
        1000;

    status->valid = true;
    status->raw = raw_average;
    status->adc_voltage_mv = adc_voltage_mv;
    status->voltage_mv = battery_voltage_mv;
    status->percent = clamp_percent(battery_percent_from_mv(battery_voltage_mv));

    ESP_LOGI(TAG, "Battery ADC: %d%%, battery=%d mV, adc=%d mV, raw=%d",
             status->percent, status->voltage_mv, status->adc_voltage_mv,
             status->raw);
    return ESP_OK;
}

#else

esp_err_t smonitor_battery_init(void)
{
    return ESP_OK;
}

esp_err_t smonitor_battery_read(smonitor_battery_status_t *status)
{
    ESP_RETURN_ON_FALSE(status != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Battery status output is required");
    memset(status, 0, sizeof(*status));
    return ESP_ERR_NOT_SUPPORTED;
}

#endif
