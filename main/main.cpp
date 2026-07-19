#include "freertos/FreeRTOS.h"
#include "radio_service.h"
#include "esp_log.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Booting up...");

    RadioService radioService;

    int state = radioService.begin();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Radio failed to initialize, code %d\n", state);
    }

    state = radioService.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Failed to start receiving, code %d\n", state);
    }

    while (true) {
        ESP_LOGI(TAG, "Waiting for packet...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
