#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>
#include "time_sync.hpp"

static const char *TAG = "time_sync";

void init_time_sync(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

bool wait_for_time_sync(int max_retries)
{
    time_t now = 0;
    struct tm timeinfo = {};

    int retry = 0;

    while (timeinfo.tm_year < (2016 - 1900) && retry++ < max_retries) {
        ESP_LOGI(TAG, "Waiting for system time... (%d/%d)", retry, max_retries);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGW(TAG, "System time not set yet.");
        return false;
    }

    ESP_LOGI(TAG, "System time set.");
    return true;
}
