#pragma once

#include "freertos/FreeRTOS.h"
#include "radio_service.h"
#include "device_config.h"
#include "packet.h"

namespace app {

constexpr const uint8_t kPacketScreenerBufferSize = 100;

struct ButtonContext {
    QueueHandle_t queue;
};

enum class ScreenerResult {
    SCR_OK,
    SCR_ERROR_DUPLICATE,
    SCR_ERROR_WRONG_GROUP
};

class PacketScreener {
    uint64_t current_group_id;
    protocol::MessageId newest_packets_seen[kPacketScreenerBufferSize] = {};
    uint8_t seen_unit_count = 0;
public:
    PacketScreener(uint64_t group_id);
    ScreenerResult screen_packet(const protocol::Packet& packet);
};

class AppController {
    TaskHandle_t app_task_handle = nullptr;
    QueueHandle_t app_queue_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 0;
    device::DeviceConfig device_config;

    radio::RadioService radio;
        
    // #if CONFIG_MESHENGER_DEVICE_ID_A
    //     static constexpr char kUnitId = 'A';
    // #elif CONFIG_MESHENGER_DEVICE_ID_B
    //     static constexpr char kUnitId = 'B';
    // #endif

    gpio_num_t button_pin = GPIO_NUM_0;
    button_t main_btn;
    ButtonContext btn_ctx;

    static void app_task(void* pvParameters);
    void switch_group();
public:
    AppController();
    void init();
    void send_status_update();
    void status_update_received(const uint8_t* serialized_packet);
};

}
