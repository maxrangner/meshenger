#pragma once

#include "freertos/FreeRTOS.h"
#include "radio_service.h"

namespace app {

enum class AppEvent {
    SEND_MESSAGE,
    MESSAGE_RECEIVED
};

struct ButtonContext {
    QueueHandle_t queue;
};

class AppController {
    TaskHandle_t app_task_handle = nullptr;
    QueueHandle_t app_queue_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 0;

    radio::RadioService radio;
    QueueHandle_t radio_queue_handle;

    gpio_num_t button_pin = GPIO_NUM_0;
    button_t main_btn;
    ButtonContext btn_ctx;

    static void app_task(void* pvParameters);
public:
    AppController();
    void init();
};

}
