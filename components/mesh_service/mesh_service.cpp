#include "mesh_service.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "utils.h"
#include "app_message.h"
#include "device_config_storage.h"

static const char *TAG = "MeshService";

namespace mesh {

MeshService::MeshService() {}

void IRAM_ATTR MeshService::irq_event() {
    mesh::MeshCommand command;
    command.message = mesh::MeshCommandMessage::RADIO_EVENT;

    xQueueSendFromISR(mesh_queue_handle, &command, nullptr);
}

void MeshService::init(QueueHandle_t app_queue) {
    mesh_queue_handle = xQueueCreate(10, sizeof(MeshCommand));
    app_queue_handle = app_queue;
    
    init_nvs();
    if (!loadDeviceSettings(&device_config)) {
        ESP_LOGI(TAG, "No stored settings loaded, using defaults");
        device_config.protocol_version = 1;
        device_config.dictionary_version = 1;
        device_config.origin_device_id = utils::get_mac_address();
        device_config.group_id = 0;
    }

    int state = radio.init(&irq_event);
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Radio failed to initialize, code %d", state);
        return;
    }

    xTaskCreatePinnedToCore(
        mesh_service_task,            // Function to implement the task
        "MeshServiceTask",            // Name of the task
        8192,                         // Stack size in bytes
        this,                         // Task input parameter
        1,                            // Priority of the task
        &mesh_task_handle,            // Task handle.
        kTaskCore                     // Core where the task should run
    );
}

void MeshService::init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void MeshService::mesh_service_task(void* pvParameters) {
    auto* self = static_cast<MeshService*>(pvParameters);

    ESP_LOGI(TAG, "Running radio task on core %d", kTaskCore);

    MeshCommand incoming_command;

    self->radio.start_rx();

    while(true) {
        xQueueReceive(self->mesh_queue_handle, &incoming_command, portMAX_DELAY);
        switch (incoming_command.message) {
            case MeshCommandMessage::SEND_PAYLOAD:
                radio::RadioResult result;
                result = self->handle_send_payload(incoming_command.payload);

                if (result != radio::RadioResult::TRANSMITTING) {
                    ESP_LOGI(TAG, "Error sending payload.");
                }

                break;
            case MeshCommandMessage::RADIO_EVENT: {
                radio::RadioResult result;
                result = self->radio.handle_irq(self->receive_buffer);

                if (result == radio::RadioResult::PACKET_RECEIVED) {
                    self->handle_packet_received(self->receive_buffer);
                } else if (result == radio::RadioResult::TRANSMIT_COMPLETE) {
                    ESP_LOGI(TAG, "Transmit complete.");
                } else if (result == radio::RadioResult::ERROR) {
                    ESP_LOGI(TAG, "Error handling IRQ from radio.");
                }

                break;
            }
        }
    }
}

void MeshService::send_payload(uint8_t *payload) {
    ESP_LOGI(TAG, "Sending payload to mesh queue...");

    MeshCommand command;
    command.message = MeshCommandMessage::SEND_PAYLOAD;
    memcpy(command.payload, payload, protocol::kPayloadSize);

    xQueueSend(mesh_queue_handle, &command, 0);
}

radio::RadioResult MeshService::handle_send_payload(uint8_t *payload) {
    protocol::Packet packet{};
    uint8_t buffer[protocol::kSerializedPacketSize];

    packet.version = protocol::kPacketVersion;
    packet.message_id.group_id = device_config.group_id;
    packet.message_id.origin_device_id = device_config.origin_device_id;
    static uint32_t num_packets = 0;
    packet.message_id.message_num = num_packets++;

    packet.payload[0] = payload[0];
    packet.payload[1] = payload[1];
    packet.payload[2] = payload[2];
    packet.payload[3] = payload[3];

    utils::serialize_packet(packet, buffer);
    radio::RadioResult result;
    result = radio.transmit(buffer);

    return result;
}
    
void MeshService::handle_packet_received(uint8_t* serialized_packet) {
    protocol::Packet packet;
    utils::deserialize_packet(serialized_packet, packet);

    app::AppEvent event;
    event.message = app::AppEventMessage::MESSAGE_RECEIVED;
    event.origin_device_id = packet.message_id.origin_device_id;
    memcpy(event.payload, packet.payload, protocol::kPayloadSize);

    xQueueSend(app_queue_handle, &event, 0);
}

}