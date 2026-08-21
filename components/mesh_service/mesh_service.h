#pragma once

#include "freertos/FreeRTOS.h"
#include "packet.h"
#include "device_state.h"
#include "radio_service.h"
#include "packet_screener.h"
#include "mesh_core.h"

namespace mesh {

enum class MeshCommandType {
    SEND_PAYLOAD,
    RADIO_IRQ,
};

struct MeshCommand {
    MeshCommandType type;
    protocol::Payload payload;
};

class MeshService {
    TaskHandle_t mesh_task_handle = nullptr;
    inline static QueueHandle_t mesh_queue_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 1;

    QueueHandle_t app_queue_handle = nullptr;

    radio::RadioService radio;
    MeshCore core;

    uint8_t receive_buffer[protocol::kSerializedPacketSize]{};

    static void IRAM_ATTR radio_irq_callback();
    static void mesh_service_task(void* pvParameters);
    radio::RadioResultType handle_send_payload(protocol::Payload& payload);
    void handle_received_frame(uint8_t* serialized_packet, size_t received_size);
    uint64_t get_mac_address();
public:
    void init(QueueHandle_t app_queue);
    void send_payload(protocol::Payload& payload);
};

}
