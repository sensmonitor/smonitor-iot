#include "device_identity.hpp"

#include <cstdio>

#include "esp_mac.h"

esp_err_t smonitor_device_serial(std::string &serial)
{
    std::array<uint8_t, 6> mac = {};
    const esp_err_t error = esp_read_mac(mac.data(), ESP_MAC_EFUSE_FACTORY);
    if (error != ESP_OK) {
        return error;
    }

    char value[32];
    snprintf(value, sizeof(value), "SM-ESP32-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    serial = value;
    return ESP_OK;
}
