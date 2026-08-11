#include "mesh_service.h"

#include "esp_log.h"
#include "esp_err.h"
#include "app_message.h"
#include "device_state_storage.h"
#include "esp_mac.h"
#include "utils.h"

static const char *TAG = "MeshService";

namespace mesh {

MeshService::MeshService() {}

void IRAM_ATTR MeshService::radio_irq_callback() {
    mesh::MeshCommand command;
    command.type = mesh::MeshCommandType::RADIO_IRQ;

    xQueueSendFromISR(mesh_queue_handle, &command, nullptr);
}

void MeshService::init(QueueHandle_t app_queue) {
    mesh_queue_handle = xQueueCreate(10, sizeof(MeshCommand));
    app_queue_handle = app_queue;
    
    DeviceState device_state{};
    if (!loadDeviceSettings(&device_state)) {
        ESP_LOGI(TAG, "No stored settings loaded, using defaults");
        device_state.protocol_version = 1;
        device_state.dictionary_version = 1;
        device_state.device_id = get_mac_address();
        device_state.group_id = 0;
        device_state.packet_count = 0;
    }
    core.set_device_state(device_state);

    int state = radio.init(&radio_irq_callback);
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

void MeshService::mesh_service_task(void* pvParameters) {
    auto* self = static_cast<MeshService*>(pvParameters);

    ESP_LOGI(TAG, "Running radio task on core %d", kTaskCore);

    MeshCommand incoming_command;
    self->radio.start_rx();

    while(true) {
        xQueueReceive(self->mesh_queue_handle, &incoming_command, portMAX_DELAY);

        switch (incoming_command.type) {
            case MeshCommandType::SEND_PAYLOAD: {
                radio::RadioResultType result = self->handle_send_payload(incoming_command.payload);
                if (result != radio::RadioResultType::TRANSMITTING) {
                    ESP_LOGI(TAG, "Error sending payload.");
                }
                break;
            }
            case MeshCommandType::RADIO_IRQ: {
                radio::RadioResult irq_result = self->radio.handle_irq(self->receive_buffer);

                if (irq_result.type == radio::RadioResultType::FRAME_RECEIVED) {
                    self->handle_received_frame(self->receive_buffer, irq_result.received_size);
                } else if (irq_result.type == radio::RadioResultType::TRANSMIT_COMPLETE) {
                    // Transmit complete
                } else if (irq_result.type == radio::RadioResultType::ERROR) {
                    // Handle error
                }
                break;
            }
        }
    }
}

void MeshService::send_payload(uint8_t *payload) {
    ESP_LOGI(TAG, "Sending payload to mesh queue...");

    MeshCommand command;
    command.type = MeshCommandType::SEND_PAYLOAD;
    memcpy(command.payload, payload, protocol::kPayloadSize);

    xQueueSend(mesh_queue_handle, &command, 0);
}

radio::RadioResultType MeshService::handle_send_payload(uint8_t *payload) {
    OutgoingResult outgoing = core.create_packet(payload);

    if (!saveDeviceSettings(outgoing.state)) {
        return radio::RadioResultType::ERROR;
    }
    
    uint8_t buffer[protocol::kSerializedPacketSize];
    protocol::utils::serialize_packet(outgoing.packet, buffer);

    return radio.transmit(buffer);
}
    
void MeshService::handle_received_frame(uint8_t* serialized_packet, size_t received_size) {
    protocol::Packet packet;
    protocol::utils::deserialize_packet(serialized_packet, packet);

    IncomingResult incoming = core.process_incoming_packet(packet);

    if (incoming.message == IncomingResultMessage::DELIVER) {
        app::AppEvent event;
        event.message = app::AppEventMessage::MESSAGE_RECEIVED;
        event.origin_device_id = packet.message_id.origin_device_id;
        memcpy(event.payload, packet.payload, protocol::kPayloadSize);
    
        xQueueSend(app_queue_handle, &event, 0);
    } else if (incoming.message == IncomingResultMessage::RELAY) {
        // Relay package
    } else {
        // Discard
    }
}

uint64_t MeshService::get_mac_address() {
    uint8_t device_mac[6];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(device_mac));
    
    uint64_t converted_mac = 0;
    for (int i = 0; i < 6; i++) {
        converted_mac = (converted_mac << 8) | device_mac[i];
    }

    return converted_mac;
}

}