#pragma once

#include "freertos/FreeRTOS.h"
#include "radio_service.h"

namespace app {

struct ButtonContext {
    QueueHandle_t queue;
};

class AppController {
    TaskHandle_t app_task_handle = nullptr;
    QueueHandle_t app_queue_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 0;

    radio::RadioService radio;
        
    #if CONFIG_MESHENGER_DEVICE_ID_A
        static constexpr char kUnitId = 'A';
    #elif CONFIG_MESHENGER_DEVICE_ID_B
        static constexpr char kUnitId = 'B';
    #endif

    gpio_num_t button_pin = GPIO_NUM_0;
    button_t main_btn;
    ButtonContext btn_ctx;

    static void app_task(void* pvParameters);
public:
    AppController();
    void init();
    void status_update();
    void status_received(const uint8_t* serialized_packet);
};

}
