#include "freertos/FreeRTOS.h"
#include "radio_service.h"
#include "packet.h"
#include "utils.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "main";

#if CONFIG_MESHENGER_ROLE_SENDER
static void runSender(RadioService *radioService) {
    int state;
    protocol::Packet packet = {};
    packet.version = 1;
    packet.senderId = 1;
    uint8_t buffer[protocol::kPacketSize] = {};

    int numPackets = 0;
    char message[32] = {};
    
    while(true) {
        snprintf(message, sizeof(message), "Send packet %d", numPackets++);
        memcpy(packet.payload, message, strlen(message) + 1);
        ESP_LOGI(TAG, "Preparing to send packet with message: %s", message);

        utils::serializePacket(packet, buffer);
        state = radioService->send(buffer, protocol::kPacketSize);

        if (state != RADIOLIB_ERR_NONE) {
            ESP_LOGI(TAG, "Failed to start sending, code %d", state);
        }
        ESP_LOGI(TAG, "Packet sent successfully");

        vTaskDelay(pdMS_TO_TICKS(CONFIG_MESHENGER_SEND_INTERVAL_MS));
    }
}
#else
static void runReceiver(RadioService *radioService) {
    int state;
    uint8_t buffer[protocol::kPacketSize] = {};
    protocol::Packet packet;

    while(true) {
        memset(buffer, 0, sizeof(buffer));

        state = radioService->receive(buffer, protocol::kPacketSize);

        if (state == RADIOLIB_ERR_NONE) {
            utils::deserializePacket(reinterpret_cast<const uint8_t*>(buffer), packet);

            ESP_LOGI(TAG, "Packet received!     Version: %d, Sender ID: %d, Message: %s", packet.version, packet.senderId, packet.payload);
        } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
            ESP_LOGI(TAG, "Receive timeout");
        } else {
            ESP_LOGI(TAG, "Failed to start receiving, code %d", state);
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
        ESP_LOGI(TAG, "Radio failed to initialize, code %d", state);
    }

    #if CONFIG_MESHENGER_ROLE_SENDER
        runSender(&radioService);
    #else
        runReceiver(&radioService);
    #endif
}
