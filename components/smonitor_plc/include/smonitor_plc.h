#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "smonitor_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SMONITOR_PLC_OPERATOR_GREATER_THAN = 0,
    SMONITOR_PLC_OPERATOR_LESS_THAN,
    SMONITOR_PLC_OPERATOR_GREATER_OR_EQUAL,
    SMONITOR_PLC_OPERATOR_LESS_OR_EQUAL,
} smonitor_plc_operator_t;

typedef struct {
    int id;
    int sensor_id;
    char sample_type[24];
    smonitor_plc_operator_t op;
    double input_value;
    int output_id;
    bool output_value;
    bool enabled;
    uint32_t revision;
} smonitor_plc_rule_t;

/**
 * Called by smonitor_plc_apply() for every fired rule, outside the PLC lock.
 * The receiver owns the outputs: it sets the GPIO and reports the change.
 */
typedef void (*smonitor_plc_output_cb_t)(int rule_id, int output_id, bool value, void *ctx);

esp_err_t smonitor_plc_init(smonitor_plc_output_cb_t output_cb, void *ctx);

/**
 * Replace the active rules. Rules with the same id and revision keep their
 * previous sample, new or changed rules fire again on their first sample.
 */
esp_err_t smonitor_plc_set_rules(const smonitor_plc_rule_t *rules, size_t rule_count);

/**
 * Evaluate the rules on one sensor read. A rule fires only when its condition
 * becomes true (threshold crossing) or on its first valid sample.
 */
esp_err_t smonitor_plc_apply(const smonitor_i2c_sample_t *samples, size_t sample_count);

bool smonitor_plc_operator_from_slug(const char *slug, smonitor_plc_operator_t *out_operator);

#ifdef __cplusplus
}
#endif
