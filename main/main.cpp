// #include "freertos/FreeRTOS.h"
#include "radio_service.h"
#include "packet.h"
#include "utils.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "main";

#if CONFIG_MESHENGER_ROLE_SENDER
static void run_sender(RadioService *radio_service) {
    int state;
    protocol::Packet packet = {};
    packet.version = 1;
    packet.sender_id = 1;
    uint8_t buffer[protocol::kPacketSize] = {};

    int num_packets = 0;
    char message[32] = {};
    
    while(true) {
        snprintf(message, sizeof(message), "Send packet %d", num_packets++);
        memcpy(packet.payload, message, strlen(message) + 1);
        ESP_LOGI(TAG, "Preparing to send packet with message: %s", message);

        utils::serialize_packet(packet, buffer);
        state = radio_service->send(buffer, protocol::kPacketSize);

        if (state != RADIOLIB_ERR_NONE) {
            ESP_LOGI(TAG, "Failed to start sending, code %d", state);
        }
        ESP_LOGI(TAG, "Packet sent successfully");

        vTaskDelay(pdMS_TO_TICKS(CONFIG_MESHENGER_SEND_INTERVAL_MS));
    }
}
#else
static void run_receiver(RadioService *radio_service) {
    int state;
    uint8_t buffer[protocol::kPacketSize] = {};
    protocol::Packet packet;

    while(true) {
        memset(buffer, 0, sizeof(buffer));

        state = radio_service->receive(buffer, protocol::kPacketSize);

        if (state == RADIOLIB_ERR_NONE) {
            utils::deserialize_packet(reinterpret_cast<const uint8_t*>(buffer), packet);

            ESP_LOGI(TAG, "Packet received!     Version: %d, Sender ID: %d, Message: %s", packet.version, packet.sender_id, packet.payload);
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

    RadioService radio_service;

    int state = radio_service.init();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Radio failed to initialize, code %d", state);
    }

    #if CONFIG_MESHENGER_ROLE_SENDER
        run_sender(&radio_service);
    #else
        run_receiver(&radio_service);
    #endif
}
