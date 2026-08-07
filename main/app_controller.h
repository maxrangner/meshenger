#pragma once

#include "freertos/FreeRTOS.h"
#include "radio_service.h"
#include "device_config.h"
#include "packet.h"
#include "mesh_service.h"
#include "button_driver.h"

namespace app {

struct ButtonContext {
    QueueHandle_t queue;
};

class AppController {
    TaskHandle_t app_task_handle = nullptr;
    QueueHandle_t app_queue_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 0;
    
    mesh::MeshService mesh;

    gpio_num_t button_pin = GPIO_NUM_0;
    button_t main_btn;
    ButtonContext btn_ctx;

    uint8_t received_message_buffer[protocol::kPayloadSize];
    uint8_t message_part_0 = 0;
    uint8_t message_part_1 = 1;

    static void app_task(void* pvParameters);
public:
    AppController();
    void init();
    void send_update();
    void handle_received_update(uint64_t origin_device_id, uint8_t *payload);
};

}
