#pragma once

#include "freertos/FreeRTOS.h"
#include "packet.h"
#include "node_state.h"
#include "radio_service.h"
#include "packet_screener.h"
#include "mesh_core.h"

namespace mesh {

enum class MeshEventType {
    SendRequested,
    RadioInterrupt,
};

struct MeshEvent {
    MeshEventType type;
    protocol::Payload payload;
};

class MeshService {
public:
    void init(QueueHandle_t app_queue);
    void send_payload(const protocol::Payload& payload);
private:
    static void IRAM_ATTR radio_irq_callback();
    static void mesh_service_task(void* pvParameters);
    radio::RadioResultType handle_send_request(const protocol::Payload& payload);
    void handle_received_frame(uint8_t* serialized_packet, const size_t received_size);
    radio::RadioResultType relay_packet(protocol::Packet& packet);
    void deliver_packet_to_app(const protocol::Packet& packet);
    uint64_t get_mac_address();
    void random_blocking_delay();

    TaskHandle_t mesh_task_handle = nullptr;
    inline static QueueHandle_t mesh_queue_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 1;

    QueueHandle_t app_queue_handle = nullptr;

    radio::RadioService radio;
    MeshCore core;

    uint8_t receive_buffer[protocol::kSerializedPacketSize]{};
};

}
