#include "app_controller.h"

#include "esp_log.h"

const char* TAG = "app_controller";

namespace app {

uint8_t kTaskCore = 0;

AppController::AppController() {
}

void AppController::init() {
    xQueueCreate(10, sizeof(AppEvent));

    xTaskCreatePinnedToCore(
        app_task,                  // Function to implement the task
        "AppTask",                 // Name of the task
        8192,                      // Stack size in bytes
        this,                      // Task input parameter
        1,                         // Priority of the task
        &app_task_handle,        // Task handle.
        kTaskCore                  // Core where the task should run
    );

    int state = radio.init();
        if (state != RADIOLIB_ERR_NONE) {
            ESP_LOGI(TAG, "Radio failed to initialize, code %d", state);
        }
}

void AppController::app_task(void* pvParameters) {
    auto* self = static_cast<AppController*>(pvParameters);

    uint32_t num_loops = 0;

    while(true) {
        ESP_LOGI(TAG, "AppTask loops: %d", num_loops++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

}