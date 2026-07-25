#pragma once

#include "freertos/FreeRTOS.h"
#include "radio_service.h"

namespace app {

enum class AppEvent {
    SEND_MESSAGE,
    MESSAGE_RECEIVED
};

class AppController {
    TaskHandle_t app_task_handle = nullptr;
    QueueHandle_t app_queue = nullptr;
    static constexpr BaseType_t kTaskCore = 0;

    radio::RadioService radio;

    static void app_task(void* pvParameters);
public:
    AppController();
    void init();
};

}
