// #include "freertos/FreeRTOS.h"
#include "radio_service.h"
#include "packet.h"
#include "utils.h"
// #include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Booting up...");

    RadioService radio_service;

    int state = radio_service.init();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Radio failed to initialize, code %d", state);
    }

    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}
