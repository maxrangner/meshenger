#pragma once

#include "freertos/FreeRTOS.h"
#include "packet.h"
#include "device_config.h"
#include "radio_service.h"
#include "packet_screener.h"

namespace mesh {

enum class MeshCommandMessage {
    SEND_PAYLOAD,
    RADIO_EVENT,
};

struct MeshCommand {
    MeshCommandMessage message;
    uint8_t payload[protocol::kPayloadSize];
};

class MeshService {
    TaskHandle_t mesh_task_handle = nullptr;
    inline static QueueHandle_t mesh_queue_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 1;

    QueueHandle_t app_queue_handle = nullptr;

    DeviceConfig device_config;
    radio::RadioService radio;
    PacketScreener screener;

    uint8_t receive_buffer[protocol::kSerializedPacketSize]{};

    static void IRAM_ATTR irq_event();
    static void mesh_service_task(void* pvParameters);
    radio::RadioResult handle_send_payload(uint8_t *payload);
    void handle_packet_received(uint8_t* serialized_packet);
public:
    MeshService();
    void init(QueueHandle_t app_queue);
    void send_payload(uint8_t *payload);
};

}
