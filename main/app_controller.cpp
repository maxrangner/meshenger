#include "app_controller.h"

#include "esp_log.h"

const char* TAG = "app_controller";

namespace app {

AppController::AppController() {
}

static void handle_button_callback(button_event_t event, gpio_num_t gpio_num, void *user_data) {
    auto* context = static_cast<ButtonContext*>(user_data);
    AppEvent button_event = AppEvent::SEND_MESSAGE;
    xQueueSend(context->queue, &button_event, 0);
}

void AppController::init() {
    ESP_LOGI(TAG, "AppController init");

    app_queue_handle = xQueueCreate(10, sizeof(AppEvent));

    int state = radio.init();
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Radio failed to initialize, code %d", state);
    }
    
    radio_queue_handle = radio.get_queue();
 
    xTaskCreatePinnedToCore(
        app_task,                  // Function to implement the task
        "AppTask",                 // Name of the task
        8192,                      // Stack size in bytes
        this,                      // Task input parameter
        1,                         // Priority of the task
        &app_task_handle,          // Task handle.
        kTaskCore                  // Core where the task should run
    );

    btn_ctx.queue = app_queue_handle;
    button_cfg_t btn_cfg = BUTTON_CFG_DEFAULT(button_pin, handle_button_callback);
    btn_cfg.user_data = &btn_ctx;
    btn_cfg.hasPullup = true;

    ESP_ERROR_CHECK(button_service_init());
    ESP_ERROR_CHECK(button_init(&btn_cfg, &main_btn));
}

void AppController::app_task(void* pvParameters) {
    auto* self = static_cast<AppController*>(pvParameters);

    ESP_LOGI(TAG, "Running app task on core %d", kTaskCore);

    AppEvent event;
    radio::RadioServiceEvent radio_event;

    while(true) {
        xQueueReceive(self->app_queue_handle, &event, portMAX_DELAY);
        switch (event) {
            case AppEvent::SEND_MESSAGE:
                radio_event = radio::RadioServiceEvent::SEND_PACKET;
                xQueueSend(self->radio_queue_handle, &radio_event, portMAX_DELAY);
                break;
            case AppEvent::MESSAGE_RECEIVED:
                break;
        }
    }
}

}