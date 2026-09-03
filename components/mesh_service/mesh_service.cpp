#include "mesh_service.h"

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "app_event.h"
#include "esp_random.h"
#include "node_state_storage.h"
#include "esp_mac.h"
#include "packet_codec.h"
#include "mesh_log.h"

constexpr char TAG[] = "mesh";

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
    const bool restored_from_nvs = load_node_state(&node_state);
    if (!restored_from_nvs) {
        node_state.device_id = get_mac_address();
        node_state.group_id = 0;
        node_state.next_sequence_num = 0;
        if (!save_node_state(node_state)) {
            ESP_LOGE(TAG, "could not store new identity - sequence numbers will restart after reboot");
        }
    }
    #ifdef CONFIG_MESHENGER_OVERRIDE_DEVICE_CONFIG
        node_state.group_id = static_cast<uint64_t>(CONFIG_MESHENGER_GROUP_ID);
        ESP_LOGW(TAG, "group id overridden by build config (development build)");
    #endif

    core.set_local_identity(node_state);
    log_node_identity(node_state, restored_from_nvs);

    int status = radio.init(&radio_irq_callback);
    if (status != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "radio init failed, code %d - mesh service not started", status);
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

    ESP_LOGD(TAG, "mesh task running on core %d", kTaskCore);

    MeshEvent event{};
    TickType_t relay_wait = portMAX_DELAY;
    self->radio.start_rx();

    while(true) {
        xQueueReceive(self->mesh_queue_handle, &event, relay_wait);

        switch (event.type) {
            case MeshEventType::SendRequested: {
                radio::RadioResultType transmit_result = self->handle_send_request(event.payload);
                if (transmit_result != radio::RadioResultType::TransmissionStarted) {
                    ESP_LOGW(TAG, "status update was not transmitted");
                }
                break;
            }
            case MeshEventType::RadioInterrupt: {
                radio::RadioResult irq_result = self->radio.handle_irq(self->receive_buffer);

                if (irq_result.type == radio::RadioResultType::FrameReceived) {
                    self->handle_received_frame(self->receive_buffer, irq_result);
                } else if (irq_result.type == radio::RadioResultType::TransmissionComplete) {
                    // Empty for now
                } else if (irq_result.type == radio::RadioResultType::Error) {
                    // Handle error
                }
                break;
            }
        }
    }
}

void MeshService::send_payload(const protocol::Payload& payload) {
    MeshEvent event;
    event.type = MeshEventType::SendRequested;
    event.payload = payload;

    if (xQueueSend(mesh_queue_handle, &event, 0) != pdTRUE) {
        ESP_LOGW(TAG, "send request dropped: mesh queue full");
        return;
    }

    ESP_LOGD(TAG, "send request queued");
}

radio::RadioResultType MeshService::handle_send_request(const protocol::Payload& payload) {
    OutgoingPacketResult outgoing = core.create_outgoing_packet(payload);

    if (!save_sequence_num(outgoing.updated_state.next_sequence_num)) {
        ESP_LOGE(TAG, "send aborted: sequence number could not be persisted");
        return radio::RadioResultType::Error;
    }

    uint8_t buffer[protocol::kSerializedPacketSize];
    protocol::codec::serialize_packet(outgoing.packet, buffer);

    radio::RadioResultType result = radio.transmit(buffer);
    if (result == radio::RadioResultType::TransmissionStarted) {
        log_own_message_sent(outgoing.packet);
    }

    return result;
}

void MeshService::handle_received_frame(uint8_t* serialized_packet, const radio::RadioResult& frame) {
    protocol::Packet packet;
    if (!protocol::codec::deserialize_packet(serialized_packet, frame.received_size, packet)) {
        log_frame_size_mismatch(frame);
        return;
    }

    IncomingPacketResult incoming = core.process_incoming_packet(packet);
    log_received_packet(incoming, frame);

    if (incoming.should_deliver) {
        deliver_packet_to_app(incoming.packet);
    }
    if (incoming.should_relay) {
        const uint32_t backoff_ms = random_blocking_delay();
        relay_packet(incoming.packet, backoff_ms);
    }
}

radio::RadioResultType MeshService::relay_packet(protocol::Packet& packet, const uint32_t backoff_ms) {
    if (!core.prepare_packet_for_relay(packet)) {
        ESP_LOGW(TAG, "relay skipped: hop limit already spent");
        return radio::RadioResultType::Error;
    }

    uint8_t transmit_buffer[protocol::kSerializedPacketSize];
    protocol::codec::serialize_packet(packet, transmit_buffer);

    radio::RadioResultType result = radio.transmit(transmit_buffer);
    if (result != radio::RadioResultType::TransmissionStarted) {
        log_relay_failed(packet);
        return result;
    }

    log_relayed_packet_sent(packet, backoff_ms);

    return result;
}

void MeshService::deliver_packet_to_app(const protocol::Packet& packet) {
    app::AppEvent event;
    event.type = app::AppEventType::StatusUpdateReceived;
    event.origin_device_id = packet.header.message_id.origin_device_id;
    event.payload = packet.payload;

    if (xQueueSend(app_queue_handle, &event, 0) != pdTRUE) {
        ESP_LOGW(TAG, "delivery dropped: app queue full");
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

uint32_t MeshService::random_blocking_delay() {
    const uint8_t min_delay_ms = 50;
    uint32_t relay_delay_ms = min_delay_ms + (esp_random() % 101);
    vTaskDelay(pdMS_TO_TICKS(relay_delay_ms));

    return relay_delay_ms;
}

}
