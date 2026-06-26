#include "websocket_client.hpp"

#include "gpio_control.hpp"
#include "json_client.hpp"
#include "smonitor_client.h"
#include "time_sync.hpp"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"
#include <cstdio>
#include <cstring>
#include <string>

static const char *TAG = "smonitor_client";

static constexpr int kConnectAttempts = 30;
static constexpr int kConnectPollMs = 1000;
static constexpr int kConfigRetryMs = 5000;

extern const uint8_t isrgrootx1_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t isrgrootx1_pem_end[] asm("_binary_isrgrootx1_pem_end");

static void sendConfigRequest();

namespace {

esp_websocket_client_handle_t client;
bool ready = true;
bool subscribed = false;
bool subscribe_sent = false;
bool config_ready = false;
bool registration_required = false;
DeviceConfig device_config;
SemaphoreHandle_t config_mutex = nullptr;
SemaphoreHandle_t config_semaphore = nullptr;
std::string incoming_payload;
std::string device_serial;
std::string reverb_channel;

void logWebsocketError(const esp_websocket_event_data_t *data)
{
    if (data == nullptr) {
        ESP_LOGE(TAG, "WEBSOCKET_EVENT_ERROR with null event data");
        return;
    }

    ESP_LOGE(TAG,
             "WEBSOCKET_EVENT_ERROR: type=%d, tls_last_esp_err=0x%x, tls_stack_err=0x%x, cert_flags=0x%x, handshake_status=%d, sock_errno=%d (%s)",
             data->error_handle.error_type,
             data->error_handle.esp_tls_last_esp_err,
             data->error_handle.esp_tls_stack_err,
             data->error_handle.esp_tls_cert_verify_flags,
             data->error_handle.esp_ws_handshake_status_code,
             data->error_handle.esp_transport_sock_errno,
             data->error_handle.esp_transport_sock_errno == 0 ? "n/a" : strerror(data->error_handle.esp_transport_sock_errno));
}

void storeDeviceConfig(DeviceConfig config)
{
    if (config_mutex == nullptr) {
        return;
    }

    applyDigitalIoConfig(config);

    xSemaphoreTake(config_mutex, portMAX_DELAY);
    device_config = config;
    config_ready = !device_config.sensors.empty();
    xSemaphoreGive(config_mutex);

    if (config_ready && config_semaphore != nullptr) {
        xSemaphoreGive(config_semaphore);
    }
}

void updateDigitalIoConfig(std::vector<DigitalIoConfig> digital_ios)
{
    if (config_mutex == nullptr) {
        return;
    }

    xSemaphoreTake(config_mutex, portMAX_DELAY);
    device_config.digital_ios = digital_ios;
    applyDigitalIoConfig(device_config);
    xSemaphoreGive(config_mutex);
}

void sendText(const char *payload)
{
    if (!esp_websocket_client_is_connected(client)) {
        return;
    }

    esp_websocket_client_send_text(client, payload, strlen(payload), 10000);
}

void sendSubscribe()
{
    if (!esp_websocket_client_is_connected(client) || subscribe_sent) {
        return;
    }

    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "{\"event\":\"pusher:subscribe\",\"data\":{\"channel\":\"%s\"}}",
             reverb_channel.c_str());
    sendText(buffer);
    subscribe_sent = true;
    ESP_LOGI(TAG, "Sent subscribe for channel: %s", reverb_channel.c_str());
}

void ensureConfigRequestFlow()
{
    if (!esp_websocket_client_is_connected(client)) {
        return;
    }

    sendSubscribe();
    sendConfigRequest();
}

void sendPong()
{
    sendText("{\"event\":\"pusher:pong\",\"data\":{}}");
}

void connectWS()
{
    esp_err_t start_err = esp_websocket_client_start(client);
    if (start_err != ESP_OK) {
        ESP_LOGE(TAG, "esp_websocket_client_start failed: %s", esp_err_to_name(start_err));
        esp_restart();
    }

    ESP_LOGI(TAG, "Connecting to %s", CONFIG_SMONITOR_WEBSOCKET_URI);
    for (int attempt = 0; attempt < kConnectAttempts; ++attempt) {
        if (esp_websocket_client_is_connected(client)) {
            ESP_LOGI(TAG, "WebSocket connected after %d second(s).", attempt);
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(kConnectPollMs));
        ESP_LOGI(TAG, ".");
    }

    ESP_LOGE(TAG, "WebSocket did not connect within %d seconds.", kConnectAttempts * kConnectPollMs / 1000);
    esp_restart();
}

bool appendWebsocketFragment(const esp_websocket_event_data_t *data, std::string &payload)
{
    if (data == nullptr || data->data_ptr == nullptr || data->data_len <= 0) {
        return false;
    }

    if (data->payload_offset == 0) {
        incoming_payload.clear();
        if (data->payload_len > 0) {
            incoming_payload.reserve(data->payload_len);
        }
    }

    incoming_payload.append(data->data_ptr, data->data_len);
    if ((data->payload_offset + data->data_len) < data->payload_len) {
        ESP_LOGI(TAG,
                 "Accumulating websocket payload: %d/%d bytes",
                 data->payload_offset + data->data_len,
                 data->payload_len);
        return false;
    }

    payload = incoming_payload;
    incoming_payload.clear();
    return true;
}

bool isRegistrationRequiredPayload(const std::string &payload)
{
    return payload.find("\"registrationRequired\":true") != std::string::npos ||
           payload.find("\\\"registrationRequired\\\":true") != std::string::npos ||
           payload.find("\"status\":\"registration_required\"") != std::string::npos ||
           payload.find("\\\"status\\\":\\\"registration_required\\\"") != std::string::npos;
}

void pauseForRegistrationRequired(int timeout_ms)
{
    ESP_LOGE(TAG, "DEVICE_NOT_REGISTERED: Device %s did not receive configuration after %d seconds.",
             device_serial.c_str(), timeout_ms / 1000);
    ESP_LOGE(TAG, "Register using:");
    ESP_LOGE(TAG, "  - Web: https://app.sensmonitor.com");
    ESP_LOGE(TAG, "  - Android: SensMonitor app from Play Store");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

} // namespace

bool hasDeviceConfig()
{
    if (config_mutex == nullptr) {
        return false;
    }

    xSemaphoreTake(config_mutex, portMAX_DELAY);
    bool has_config = config_ready;
    xSemaphoreGive(config_mutex);
    return has_config;
}

bool isRegistrationRequired()
{
    return registration_required;
}

DeviceConfig getDeviceConfig()
{
    DeviceConfig config_copy;
    if (config_mutex == nullptr) {
        return config_copy;
    }

    xSemaphoreTake(config_mutex, portMAX_DELAY);
    config_copy = device_config;
    xSemaphoreGive(config_mutex);
    return config_copy;
}

bool waitForDeviceConfig(int timeout_ms)
{
    if (config_semaphore == nullptr) {
        return false;
    }

    int waited_ms = 0;
    while (!hasDeviceConfig()) {
        if (registration_required) {
            pauseForRegistrationRequired(waited_ms);
        }
        ensureConfigRequestFlow();

        if (xSemaphoreTake(config_semaphore, pdMS_TO_TICKS(kConfigRetryMs)) == pdTRUE && hasDeviceConfig()) {
            return true;
        }

        waited_ms += kConfigRetryMs;
        if (registration_required) {
            pauseForRegistrationRequired(waited_ms);
        }
        ESP_LOGW(TAG, "Still waiting for device config after %d ms.", waited_ms);

        if (timeout_ms > 0 && waited_ms >= timeout_ms) {
            return hasDeviceConfig();
        }
    }

    return true;
}

void checkWSConnection()
{
    if (!esp_websocket_client_is_connected(client)) {
        esp_websocket_client_stop(client);
        subscribed = false;
        subscribe_sent = false;
        incoming_payload.clear();
        connectWS();
    }
}

static void sendConfigRequest()
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "{\"event\":\"device-config-request\",\"channel\":\"%s\",\"data\":{\"deviceId\":\"%s\"}}",
             reverb_channel.c_str(), device_serial.c_str());
    sendText(buffer);
    ESP_LOGI(TAG, "Sent device-config-request for: %s", device_serial.c_str());
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = static_cast<esp_websocket_event_data_t *>(event_data);
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        subscribed = false;
        subscribe_sent = false;
        incoming_payload.clear();
        ensureConfigRequestFlow();
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        checkWSConnection();
        break;
    case WEBSOCKET_EVENT_DATA: {
        ready = true;
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n",
                 data->payload_len, data->data_len, data->payload_offset);

        if (data->op_code != 0x01 || data->data_len <= 0) {
            break;
        }

        std::string payload;
        if (!appendWebsocketFragment(data, payload)) {
            break;
        }

        if (payload.size() <= 512) {
            ESP_LOGW(TAG, "Received=%s", payload.c_str());
        } else {
            ESP_LOGW(TAG, "Received complete payload of %d bytes", static_cast<int>(payload.size()));
        }

        if (payload.find("\"event\":\"pusher:connection_established\"") != std::string::npos) {
            ensureConfigRequestFlow();
        } else if (payload.find("\"event\":\"pusher_internal:subscription_succeeded\"") != std::string::npos) {
            subscribed = true;
            ESP_LOGI(TAG, "Subscription succeeded.");
            sendConfigRequest();
        } else if (payload.find("\"event\":\"pusher:ping\"") != std::string::npos) {
            sendPong();
        } else if (payload.find("\"event\":\"device-sample-ack\"") != std::string::npos) {
            ready = true;
            ESP_LOGI(TAG, "Received device-sample-ack.");
            std::vector<DigitalIoConfig> digital_ios;
            if (handleDigitalIoConfigPayload(payload, digital_ios)) {
                updateDigitalIoConfig(digital_ios);
                ESP_LOGI(TAG, "Applied digital I/O config from ack with %d rows.", static_cast<int>(digital_ios.size()));
            }
        } else if (isRegistrationRequiredPayload(payload)) {
            registration_required = true;
            if (config_semaphore != nullptr) {
                xSemaphoreGive(config_semaphore);
            }
        } else {
            DeviceConfig parsed_config;
            if (handleDeviceConfigPayload(payload, parsed_config)) {
                registration_required = false;
                storeDeviceConfig(parsed_config);
                ESP_LOGI(TAG, "Received device config with %d active sensors.", static_cast<int>(parsed_config.sensors.size()));
            } else {
                ESP_LOGW(TAG, "Received payload that is not a usable config event.");
                ESP_LOGW(TAG, "Full payload follows:");
                ESP_LOGW(TAG, "%s", payload.c_str());
            }
        }
        break;
    }
    case WEBSOCKET_EVENT_ERROR:
        logWebsocketError(data);
        break;
    }
}

void initWSClient(const std::string &serial)
{
    device_serial = serial;
    reverb_channel = "device-" + serial;
    if (config_mutex == nullptr) {
        config_mutex = xSemaphoreCreateMutex();
    }
    if (config_semaphore == nullptr) {
        config_semaphore = xSemaphoreCreateBinary();
    }

    esp_websocket_client_config_t websocket_cfg = {};
    websocket_cfg.uri = CONFIG_SMONITOR_WEBSOCKET_URI;
    websocket_cfg.ping_interval_sec = -1;
    websocket_cfg.keep_alive_enable = false;
    websocket_cfg.keep_alive_interval = 0;
    websocket_cfg.keep_alive_idle = 0;
    websocket_cfg.reconnect_timeout_ms = 30000;
    websocket_cfg.network_timeout_ms = 30000;
    websocket_cfg.disable_auto_reconnect = true;
    websocket_cfg.cert_pem = reinterpret_cast<const char *>(isrgrootx1_pem_start);

    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, static_cast<void *>(client));

    connectWS();
}

void sendData(std::string data)
{
    if (esp_websocket_client_is_connected(client)) {
        std::string payload = std::string("{\"event\":\"device-sample\",\"channel\":\"") + reverb_channel + "\",\"data\":" + data + "}";
        ready = false;
        esp_websocket_client_send_text(client, payload.c_str(), payload.length(), 10000);
        ESP_LOGI(TAG, "Sent data: %s", payload.c_str());

        int attempt = 0;
        while (!ready && (attempt++) < 1000) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        if (!ready) {
            esp_restart();
        }
    }
}

void sendSample(const smonitor_i2c_sample_t *samples, std::size_t sample_count,
                int battery_percent,
                const smonitor_client_location_t &location)
{
    if (registration_required) {
        ESP_LOGW(TAG, "Skipping sample: device registration is required.");
        return;
    }
    if (!waitForDeviceConfig(60000)) {
        ESP_LOGW(TAG, "Skipping sample because device config has not arrived yet.");
        return;
    }

    std::string json =
        formatJSON(samples, sample_count, getDeviceConfig(), device_serial,
                   battery_percent, location);
    sendData(json);
}

extern "C" esp_err_t smonitor_client_start(const char *serial)
{
    if (serial == nullptr || serial[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    init_time_sync();
    wait_for_time_sync(CONFIG_SMONITOR_CLIENT_TIME_SYNC_RETRIES);
    initWSClient(serial);
    return ESP_OK;
}

extern "C" bool smonitor_client_wait_for_device_config(int timeout_ms)
{
    return waitForDeviceConfig(timeout_ms);
}

extern "C" smonitor_client_status_t smonitor_client_status(void)
{
    if (registration_required) {
        return SMONITOR_CLIENT_STATUS_DEVICE_NOT_REGISTERED;
    }
    if (hasDeviceConfig()) {
        return SMONITOR_CLIENT_STATUS_READY;
    }
    if (client == nullptr || !esp_websocket_client_is_connected(client)) {
        return SMONITOR_CLIENT_STATUS_DISCONNECTED;
    }
    return SMONITOR_CLIENT_STATUS_STARTING;
}

extern "C" esp_err_t smonitor_client_send_samples(
    const smonitor_i2c_sample_t *samples,
    size_t sample_count,
    int battery_percent,
    const smonitor_client_location_t *location)
{
    if (samples == nullptr && sample_count > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (location == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (registration_required || !hasDeviceConfig()) {
        return ESP_ERR_INVALID_STATE;
    }

    sendSample(samples, sample_count, battery_percent, *location);
    return ESP_OK;
}

extern "C" void smonitor_client_process(void)
{
    checkWSConnection();
}
