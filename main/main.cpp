#include "freertos/FreeRTOS.h"
#include "radio_service.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "main";

#if CONFIG_MESHENGER_ROLE_SENDER
static void runSender(RadioService *radioService) {
    int state;
    int numPackets = 0;
    char message[32] = {};
    while(true) {
        snprintf(message, sizeof(message), "Send packet %d", numPackets++);
        ESP_LOGI(TAG, "Preparing to send packet: %s\n", message);

        state = radioService->send(message, strlen(message));
        if (state != RADIOLIB_ERR_NONE) {
            ESP_LOGI(TAG, "Failed to start sending, code %d\n", state);
        }
        vTaskDelay(pdMS_TO_TICKS(CONFIG_MESHENGER_SEND_INTERVAL_MS));
    }
}
#else
static void runReceiver(RadioService *radioService) {
    int state;
    char buffer[33] = {};

    while(true) {
        memset(buffer, 0, sizeof(buffer));

        state = radioService->receive(buffer, sizeof(buffer) - 1);

        if (state == RADIOLIB_ERR_NONE) {
            buffer[sizeof(buffer) - 1] = '\0';
            ESP_LOGI(TAG, "Received packet: %s\n", buffer);
        } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
            ESP_LOGI(TAG, "Receive timeout\n");
        } else {
            ESP_LOGI(TAG, "Failed to start receiving, code %d\n", state);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Booting up...");

    RadioService radioService;

    int state = radioService.begin();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Radio failed to initialize, code %d\n", state);
    }

    #if CONFIG_MESHENGER_ROLE_SENDER
        runSender(&radioService);
    #else
        runReceiver(&radioService);
    #endif
}
