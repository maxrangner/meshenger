#include "mesh_service.h"

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "app_event.h"
#include "esp_random.h"
#include "node_state_storage.h"
#include "esp_mac.h"
#include "packet_codec.h"

constexpr char TAG[] = "mesh_service";

namespace mesh {

void IRAM_ATTR MeshService::radio_irq_callback() {
    mesh::MeshEvent event;
    event.type = mesh::MeshEventType::RadioInterrupt;

    xQueueSendFromISR(mesh_queue_handle, &event, nullptr);
}

void MeshService::init(QueueHandle_t app_queue) {
    mesh_queue_handle = xQueueCreate(10, sizeof(MeshEvent));
    app_queue_handle = app_queue;
    
    LocalNodeState node_state{};
    if (!load_node_state(&node_state)) {
        ESP_LOGI(TAG, "No stored settings loaded, using defaults");
        node_state.protocol_version = 1;
        node_state.phrase_dictionary_version = 1;
        node_state.device_id = get_mac_address();
        node_state.group_id = 0;
        node_state.next_sequence_num = 0;
    }
    #ifdef CONFIG_MESHENGER_OVERRIDE_DEVICE_CONFIG
        node_state.group_id = static_cast<uint64_t>(CONFIG_MESHENGER_GROUP_ID);
    #endif

    core.set_local_identity(node_state);

    int status = radio.init(&radio_irq_callback);
    if (status != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Radio failed to initialize, code %d", status);
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

    MeshEvent event;
    TickType_t relay_wait = portMAX_DELAY;
    self->radio.start_rx();

    while(true) {
        xQueueReceive(self->mesh_queue_handle, &event, relay_wait);

        switch (event.type) {
            case MeshEventType::SendRequested: {
                radio::RadioResultType transmit_result = self->handle_send_request(event.payload);
                if (transmit_result != radio::RadioResultType::TransmissionStarted) {
                    ESP_LOGI(TAG, "Error sending payload.");
                }
                break;
            }
            case MeshEventType::RadioInterrupt: {
                radio::RadioResult irq_result = self->radio.handle_irq(self->receive_buffer);

                if (irq_result.type == radio::RadioResultType::FrameReceived) {
                    self->handle_received_frame(self->receive_buffer, irq_result.received_size);
                } else if (irq_result.type == radio::RadioResultType::TransmissionComplete) {
                    LocalNodeState local_state = self->core.get_node_state();
                    save_node_state(local_state);
                } else if (irq_result.type == radio::RadioResultType::Error) {
                    // Handle error
                }
                break;
            }
        }
    }
}

void MeshService::send_payload(const protocol::Payload& payload) {
    ESP_LOGI(TAG, "Sending payload to mesh queue...");

    MeshEvent event;
    event.type = MeshEventType::SendRequested;
    event.payload = payload;

    xQueueSend(mesh_queue_handle, &event, 0);
}

radio::RadioResultType MeshService::handle_send_request(const protocol::Payload& payload) {
    OutgoingPacketResult outgoing = core.create_outgoing_packet(payload);

    if (!save_node_state(outgoing.updated_state)) {
        return radio::RadioResultType::Error;
    }
    
    uint8_t buffer[protocol::kSerializedPacketSize];
    protocol::codec::serialize_packet(outgoing.packet, buffer);

    return radio.transmit(buffer);
}
    
void MeshService::handle_received_frame(uint8_t* serialized_packet, const size_t received_size) {
    protocol::Packet packet;
    if (!protocol::codec::deserialize_packet(serialized_packet, received_size, packet)) {
        ESP_LOGW(TAG, "Frame received with incorrect size.");
        return;
    }

    IncomingPacketResult incoming = core.process_incoming_packet(packet);

    if (incoming.should_deliver) {
        deliver_packet_to_app(incoming.packet);
    }
    if (incoming.should_relay) {
        random_blocking_delay();
        relay_packet(incoming.packet);
    }
}

radio::RadioResultType MeshService::relay_packet(protocol::Packet& packet) {
    if (!core.prepare_packet_for_relay(packet)) {
        return {radio::RadioResultType::Error};
    }
    
    uint8_t transmit_buffer[protocol::kSerializedPacketSize];
    protocol::codec::serialize_packet(packet, transmit_buffer);

    return radio.transmit(transmit_buffer);
}

void MeshService::deliver_packet_to_app(const protocol::Packet& packet) {
    app::AppEvent event;
    event.type = app::AppEventType::StatusUpdateReceived;
    event.origin_device_id = packet.header.message_id.origin_device_id;
    event.payload = packet.payload;

    xQueueSend(app_queue_handle, &event, 0);
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

void MeshService::random_blocking_delay() {
    const uint8_t min_delay_ms = 50;
    uint8_t relay_delay_ms = min_delay_ms + (esp_random() % 101);
    vTaskDelay(pdMS_TO_TICKS(relay_delay_ms)); 
}

}
