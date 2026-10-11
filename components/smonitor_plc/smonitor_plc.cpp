#include "smonitor_plc.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace {

constexpr const char *TAG = "smonitor_plc";

struct RuleState {
    smonitor_plc_rule_t rule;
    bool has_previous;
    double previous_value;
};

struct OutputAction {
    int rule_id;
    int output_id;
    bool value;
};

SemaphoreHandle_t rules_mutex = nullptr;
std::vector<RuleState> rule_states;
smonitor_plc_output_cb_t output_callback = nullptr;
void *output_callback_ctx = nullptr;

bool ensureMutex()
{
    if (rules_mutex == nullptr) {
        rules_mutex = xSemaphoreCreateMutex();
    }

    return rules_mutex != nullptr;
}

const char *operatorSymbol(smonitor_plc_operator_t op)
{
    switch (op) {
    case SMONITOR_PLC_OPERATOR_GREATER_THAN:
        return ">";
    case SMONITOR_PLC_OPERATOR_LESS_THAN:
        return "<";
    case SMONITOR_PLC_OPERATOR_GREATER_OR_EQUAL:
        return ">=";
    case SMONITOR_PLC_OPERATOR_LESS_OR_EQUAL:
        return "<=";
    default:
        return "?";
    }
}

bool compareValue(smonitor_plc_operator_t op, double value, double threshold)
{
    switch (op) {
    case SMONITOR_PLC_OPERATOR_GREATER_THAN:
        return value > threshold;
    case SMONITOR_PLC_OPERATOR_LESS_THAN:
        return value < threshold;
    case SMONITOR_PLC_OPERATOR_GREATER_OR_EQUAL:
        return value >= threshold;
    case SMONITOR_PLC_OPERATOR_LESS_OR_EQUAL:
        return value <= threshold;
    default:
        return false;
    }
}

// Same lookup as the sample payload: first valid sample of the sensor type.
const smonitor_i2c_sample_t *findSample(const smonitor_i2c_sample_t *samples,
                                        size_t sample_count,
                                        const char *sample_type)
{
    for (size_t index = 0; index < sample_count; ++index) {
        if (samples[index].valid && std::strcmp(samples[index].type, sample_type) == 0) {
            return &samples[index];
        }
    }

    return nullptr;
}

} // namespace

extern "C" esp_err_t smonitor_plc_init(smonitor_plc_output_cb_t output_cb, void *ctx)
{
    if (!ensureMutex()) {
        return ESP_ERR_NO_MEM;
    }

    xSemaphoreTake(rules_mutex, portMAX_DELAY);
    output_callback = output_cb;
    output_callback_ctx = ctx;
    xSemaphoreGive(rules_mutex);
    return ESP_OK;
}

extern "C" esp_err_t smonitor_plc_set_rules(const smonitor_plc_rule_t *rules, size_t rule_count)
{
    if (rules == nullptr && rule_count > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!ensureMutex()) {
        return ESP_ERR_NO_MEM;
    }

    std::vector<RuleState> next_states;
    next_states.reserve(rule_count);

    xSemaphoreTake(rules_mutex, portMAX_DELAY);
    bool changed = rule_count != rule_states.size();

    for (size_t index = 0; index < rule_count; ++index) {
        RuleState state = {rules[index], false, 0.0};

        auto previous = std::find_if(rule_states.begin(), rule_states.end(), [&](const RuleState &item) {
            return item.rule.id == rules[index].id;
        });

        if (previous != rule_states.end() && previous->rule.revision == rules[index].revision) {
            state.has_previous = previous->has_previous;
            state.previous_value = previous->previous_value;
        } else {
            changed = true;
        }

        next_states.push_back(state);
    }

    // Rules are applied by id, so the last matching rule wins for the same output.
    std::sort(next_states.begin(), next_states.end(), [](const RuleState &left, const RuleState &right) {
        return left.rule.id < right.rule.id;
    });

    rule_states = next_states;
    xSemaphoreGive(rules_mutex);

    if (changed) {
        ESP_LOGI(TAG, "Loaded %d PLC rule(s).", static_cast<int>(rule_count));
        for (const RuleState &state : next_states) {
            ESP_LOGI(TAG, "  rule #%d: %s %s %.2f -> output %d %s%s",
                     state.rule.id,
                     state.rule.sample_type,
                     operatorSymbol(state.rule.op),
                     state.rule.input_value,
                     state.rule.output_id,
                     state.rule.output_value ? "ON" : "OFF",
                     state.rule.enabled ? "" : " (disabled)");
        }
    }

    return ESP_OK;
}

extern "C" esp_err_t smonitor_plc_apply(const smonitor_i2c_sample_t *samples, size_t sample_count)
{
    if (samples == nullptr && sample_count > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!ensureMutex()) {
        return ESP_ERR_NO_MEM;
    }

    std::vector<OutputAction> actions;

    xSemaphoreTake(rules_mutex, portMAX_DELAY);
    smonitor_plc_output_cb_t callback = output_callback;
    void *callback_ctx = output_callback_ctx;

    for (RuleState &state : rule_states) {
        const smonitor_plc_rule_t &rule = state.rule;
        if (!rule.enabled) {
            continue;
        }

        const smonitor_i2c_sample_t *sample = findSample(samples, sample_count, rule.sample_type);
        if (sample == nullptr || std::isnan(sample->value)) {
            // A failed read keeps the previous sample, so it never fakes a crossing.
            continue;
        }

        const double value = sample->value;
        const bool matches = compareValue(rule.op, value, rule.input_value);
        const bool matched = state.has_previous && compareValue(rule.op, state.previous_value, rule.input_value);

        state.has_previous = true;
        state.previous_value = value;

        if (!matches || matched) {
            continue;
        }

        ESP_LOGI(TAG, "PLC rule #%d fired: %s %.2f %s %.2f -> output %d %s",
                 rule.id,
                 rule.sample_type,
                 value,
                 operatorSymbol(rule.op),
                 rule.input_value,
                 rule.output_id,
                 rule.output_value ? "ON" : "OFF");
        actions.push_back({rule.id, rule.output_id, rule.output_value});
    }
    xSemaphoreGive(rules_mutex);

    if (callback == nullptr) {
        if (!actions.empty()) {
            ESP_LOGW(TAG, "PLC output callback is not registered; %d action(s) dropped.",
                     static_cast<int>(actions.size()));
        }
        return ESP_OK;
    }

    for (const OutputAction &action : actions) {
        callback(action.rule_id, action.output_id, action.value, callback_ctx);
    }

    return ESP_OK;
}

extern "C" bool smonitor_plc_operator_from_slug(const char *slug, smonitor_plc_operator_t *out_operator)
{
    if (slug == nullptr || out_operator == nullptr) {
        return false;
    }

    if (std::strcmp(slug, "greater_than") == 0) {
        *out_operator = SMONITOR_PLC_OPERATOR_GREATER_THAN;
    } else if (std::strcmp(slug, "less_than") == 0) {
        *out_operator = SMONITOR_PLC_OPERATOR_LESS_THAN;
    } else if (std::strcmp(slug, "greater_or_equal") == 0) {
        *out_operator = SMONITOR_PLC_OPERATOR_GREATER_OR_EQUAL;
    } else if (std::strcmp(slug, "less_or_equal") == 0) {
        *out_operator = SMONITOR_PLC_OPERATOR_LESS_OR_EQUAL;
    } else {
        return false;
    }

    return true;
}
