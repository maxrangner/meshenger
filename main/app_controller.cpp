#include "app_controller.h"

#include "esp_mac.h"
#include "button_driver.h"
#include "esp_log.h"
#include "utils.h"
#include "app_message.h"

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

    uint8_t device_mac[6];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(device_mac));
    device_id = utils::mac_converter(device_mac);

    app_queue_handle = xQueueCreate(10, sizeof(AppEvent));

    int state = radio.init(app_queue_handle);
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Radio failed to initialize, code %d", state);
    }
 
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

    while(true) {
        xQueueReceive(self->app_queue_handle, &event, portMAX_DELAY);
        switch (event.message) {
            case AppEventMessage::BUTTON_SHORT_PRESS:
                self->status_update();
                break;
            case AppEventMessage::BUTTON_LONG_PRESS:
                ESP_LOGI(TAG, "Button long press has no action yet.");
                break;
            case AppEventMessage::MESSAGE_RECEIVED:
                self->status_received(event.payload);
                break;
            default:
                break;
        }
    }
}

void AppController::status_update() {
    ESP_LOGI(TAG, "Update status.");

    protocol::Packet packet;
    uint8_t buffer[protocol::kPacketSize];

    packet.device_id = kUnitId;
    packet.version = protocol::kPacketVersion;
    char message[protocol::kPayloadSize] = {};
    static uint16_t num_packets = 0;

    snprintf(message, sizeof(message), "Unit %c -Packet(v.%d)#%d", kUnitId, packet.version, num_packets++);
    memcpy(packet.payload, message, protocol::kPayloadSize);

    utils::serialize_packet(packet, buffer);

    radio.send_packet(buffer);
}

void AppController::status_received(const uint8_t* serialized_packet) {
    ESP_LOGI(TAG, "Status received.");

    protocol::Packet packet;
    utils::deserialize_packet(serialized_packet, packet);

    ESP_LOGI(TAG, "Packet received: Unit %c (v.%d): %s", packet.device_id, packet.version, packet.payload);
}

}