#include "app_controller.h"

#include "esp_mac.h"
#include "button_driver.h"
#include "esp_log.h"
#include "utils.h"
#include "app_message.h"
#include "packet_screener.h"

const char* TAG = "app_controller";

namespace app {

PacketScreener::PacketScreener(uint64_t group_id): current_group_id(group_id) {}

ScreenerResult PacketScreener::screen_packet(const protocol::Packet& packet) {
    ScreenerResult result;

    for (int i = 0; i < seen_unit_count; i++) {
        if (packet.message_id.group_id == newest_packets_seen[i].group_id && packet.message_id.origin_device_id == newest_packets_seen[i].origin_device_id) {
            if (packet.message_id.message_num > newest_packets_seen[i].message_num) {
                newest_packets_seen[i] = packet.message_id;
                return ScreenerResult::SCR_OK;
            } else {
                return ScreenerResult::SCR_ERROR_DUPLICATE;
            }
        }
    }

    if (seen_unit_count < kPacketScreenerBufferSize) {
        newest_packets_seen[seen_unit_count++] = packet.message_id;
    }

    return ScreenerResult::SCR_OK;;
}

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

    device_config.origin_device_id = utils::get_mac_address();
    device_config.group_id = 0;

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
                self->send_status_update();
                break;
            case AppEventMessage::BUTTON_LONG_PRESS:
                self->switch_group();
                ESP_LOGI(TAG, "Switching group to: %llu.", self->device_config.group_id);
                break;
            case AppEventMessage::MESSAGE_RECEIVED:
                self->status_update_received(event.payload);
                break;
            default:
                break;
        }
    }
}

void AppController::switch_group() {
    if (device_config.group_id == 0) {
        device_config.group_id = 1;
    } else {
        device_config.group_id = 0;
    }
}

void AppController::send_status_update() {
    ESP_LOGI(TAG, "Update status.");

    protocol::Packet packet;
    uint8_t buffer[protocol::kPacketSize];

    packet.version = protocol::kPacketVersion;
    packet.message_id.group_id = device_config.group_id;
    packet.message_id.origin_device_id = device_config.origin_device_id;
    char message[protocol::kPayloadSize] = {};
    static uint16_t num_packets = 0;
    packet.message_id.message_num = num_packets++;

    snprintf(message, sizeof(message), "%s", "Hello world!");
    memcpy(packet.payload, message, protocol::kPayloadSize);

    utils::serialize_packet(packet, buffer);

    radio.send_packet(buffer);
}

void AppController::status_update_received(const uint8_t* serialized_packet) {
    ESP_LOGI(TAG, "Status received.");

    protocol::Packet packet;
    utils::deserialize_packet(serialized_packet, packet);

    if (protocol::is_packet_for_group(device_config.group_id, packet)) {
        ESP_LOGI(TAG, "Packet received from unit %llx in group: %llu (message.id: %lu protocol v.%d): %s", packet.message_id.origin_device_id, packet.message_id.group_id, packet.message_id.message_num, packet.version, packet.payload);
    } else {
        ESP_LOGI(TAG, "Packet from other group received. Relaying.");
        // Relay packet
    }
}

}