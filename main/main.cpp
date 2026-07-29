// #include "freertos/FreeRTOS.h"
// #include "radio_service.h"
#include "app_controller.h"
#include "packet.h"
#include "utils.h"
#include "esp_log.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Booting up...");

    app::AppController app_controller;

    app_controller.init();

    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}
