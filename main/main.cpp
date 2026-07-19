#include "freertos/FreeRTOS.h"
#include "radio_service.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "main";

static void runSender(RadioService *radioService) {
    int state = radioService->startSend();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Failed to start sending, code %d\n", state);
    }
    while(true) {

    }
}

static void runReceiver(RadioService *radioService) {
    int state = radioService->startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Failed to start receiving, code %d\n", state);
    }
    while(true) {

    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Booting up...");

    RadioService radioService;

    int state = radioService.begin();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Radio failed to initialize, code %d\n", state);
    }

    #if CONFIG_MESHERNGER_ROLE_SENDER
        runSender(&radioService);
    #else
        runReceiver(&radioService);
    #endif
}
