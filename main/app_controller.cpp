#include "app_controller.h"

#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "packet_codec.h"
#include "app_event.h"
#include "packet_screener.h"
#include "preset_phrase_dictionary.h"


constexpr char TAG[] = "app_controller";

namespace app {

static void handle_button_callback(button_event_t event, gpio_num_t gpio_num, void *user_data) {
    auto* context = static_cast<ButtonContext*>(user_data);

    AppEvent button_event;

    switch (event) {
        case button_event_t::BTN_SHORT_PRESS:
            ESP_LOGI(TAG, "Button callback - Short press!");

            button_event.type = AppEventType::ShortButtonPress;
            xQueueSend(context->queue, &button_event, 0);
            break;
        case button_event_t::BTN_LONG_PRESS:
            ESP_LOGI(TAG, "Button callback - Long press!");

            button_event.type = AppEventType::LongButtonPress;
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
        switch (event.type) {
            case AppEventType::ShortButtonPress:
                self->send_status_update();
                break;
            case AppEventType::LongButtonPress:
                ESP_LOGI(TAG, "No function set for long press.");
                break;
            case AppEventType::StatusUpdateReceived:
                self->handle_received_status_update(event.origin_device_id, event.payload);
                break;
            default:
                break;
        }
    }
}

void AppController::send_status_update() {
    // TODO: Construct status update through UI

    protocol::Payload payload{};
    payload.bytes[0] = 2; // Message length
    payload.bytes[1] = message_part_0; // Phrase one
    payload.bytes[2] = message_part_1; // Phrase two

    ESP_LOGI(TAG, "Send payload action: %s, %s", kPhraseDictionaryV1[message_part_0], kPhraseDictionaryV1[message_part_1]);

    mesh.send_payload(payload);
}

void AppController::handle_received_status_update(const uint64_t origin_device_id, const protocol::Payload payload) {
    ESP_LOGI(TAG, "New status update received!");
    ESP_LOGI(TAG, "Message: %s, %s", kPhraseDictionaryV1[payload.bytes[1]], kPhraseDictionaryV1[payload.bytes[2]]);
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
