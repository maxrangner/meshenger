#include "app_controller.h"

#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "utils.h"
#include "app_message.h"
#include "packet_screener.h"
#include "preset_messages.h"


const char* TAG = "app_controller";

namespace app {

AppController::AppController() {
}

static void handle_button_callback(button_event_t event, gpio_num_t gpio_num, void *user_data) {
    auto* context = static_cast<ButtonContext*>(user_data);

    AppEvent button_event;

    switch (event) {
        case button_event_t::BTN_SHORT_PRESS:
            ESP_LOGI(TAG, "Button callback - Short press!");

            button_event.message = AppEventMessage::BUTTON_SHORT_PRESS;
            xQueueSend(context->queue, &button_event, 0);
            break;
        case button_event_t::BTN_LONG_PRESS:
            ESP_LOGI(TAG, "Button callback - Long press!");

            button_event.message = AppEventMessage::BUTTON_LONG_PRESS;
            xQueueSend(context->queue, &button_event, 0);
            break;
    }
}

void AppController::init() {
    ESP_LOGI(TAG, "AppController init");

    init_nvs();

    app_queue_handle = xQueueCreate(10, sizeof(AppEvent));
    xTaskCreatePinnedToCore(
        app_task,                  // Function to implement the task
        "AppTask",                 // Name of the task
        8192,                      // Stack size in bytes
        this,                      // Task input parameter
        1,                         // Priority of the task
        &app_task_handle,          // Task handle.
        kTaskCore                  // Core where the task should run
    );

    mesh.init(app_queue_handle);

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

    while(true) {
        xQueueReceive(self->app_queue_handle, &event, portMAX_DELAY);
        switch (event.message) {
            case AppEventMessage::BUTTON_SHORT_PRESS:
                self->send_update();
                break;
            case AppEventMessage::BUTTON_LONG_PRESS:
                ESP_LOGI(TAG, "No function set for long press.");
                break;
            case AppEventMessage::MESSAGE_RECEIVED:
                self->handle_received_update(event.origin_device_id, event.payload);
                break;
            default:
                break;
        }
    }
}

void AppController::send_update() {
    // TODO: Construct status update through UI

    uint8_t payload[protocol::kPayloadSize]{};
    payload[0] = 2; // Message length
    payload[1] = message_part_0; // Phrase one
    payload[2] = message_part_1; // Phrase two

    ESP_LOGI(TAG, "Send payload message: %s, %s", message_parts[message_part_0], message_parts[message_part_1]);

    mesh.send_payload(payload);
}

void AppController::handle_received_update(uint64_t origin_device_id, uint8_t *payload) {
    ESP_LOGI(TAG, "New status update received!");
    ESP_LOGI(TAG, "Message: %s, %s", message_parts[payload[1]], message_parts[payload[2]]);
}

void AppController::init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

}