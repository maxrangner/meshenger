#pragma once

#include "freertos/FreeRTOS.h"
#include "radio_service.h"
#include "device_config.h"
#include "packet.h"

namespace app {

struct ButtonContext {
    QueueHandle_t queue;
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

    uint8_t message_part_0 = 0;
    uint8_t message_part_1 = 1;

    static void app_task(void* pvParameters);
    void change_message();
    void switch_group();
public:
    AppController();
    void init();
    void send_status_update();
    void status_update_received(const uint8_t* serialized_packet);
};

}
