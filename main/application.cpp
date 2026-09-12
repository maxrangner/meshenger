#include "application.h"

#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "packet_codec.h"
#include "app_event.h"
#include "packet_screener.h"
#include "phrase_dictionary.h"
#include "app_log.h"

constexpr char TAG[] = "app";

namespace app {

static void handle_button_callback(button_event_t event, gpio_num_t gpio_num, void *user_data) {
    auto* context = static_cast<ButtonContext*>(user_data);

    AppEvent button_event;

    switch (event) {
        case button_event_t::BTN_SHORT_PRESS:
            ESP_LOGD(TAG, "button: short press");

            button_event.type = AppEventType::ShortButtonPress;
            xQueueSend(context->app_event_queue, &button_event, 0);
            break;
        case button_event_t::BTN_LONG_PRESS:
            ESP_LOGD(TAG, "button: long press");

            button_event.type = AppEventType::LongButtonPress;
            xQueueSend(context->app_event_queue, &button_event, 0);
            break;
    }
}

void Application::init() {
    init_nvs();
    init_i2c();

    oled_display.init_oled(i2c_bus_handle);
    oled_ui.show_message("Booted. Waiting for transmission...");

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
    init_btn();
    
    ESP_LOGI(TAG, "ready · short press sends a status update");
}

void Application::app_task(void* pvParameters) {
    auto* self = static_cast<Application*>(pvParameters);

    ESP_LOGD(TAG, "app task running on core %d", kTaskCore);

    AppEvent event;

    while(true) {
        xQueueReceive(self->app_queue_handle, &event, portMAX_DELAY);
        switch (event.type) {
            case AppEventType::ShortButtonPress:
                self->send_status_update();
                break;
            case AppEventType::LongButtonPress:
                ESP_LOGD(TAG, "long press has no action yet");
                break;
            case AppEventType::StatusUpdateReceived:
                self->handle_received_status_update(event.origin_device_id, event.payload);
                break;
            default:
                break;
        }
    }
}

void Application::send_status_update() {
    // TODO: Construct status update through UI

    protocol::Payload payload{};
    payload.bytes[0] = 2; // Message length
    payload.bytes[1] = message_part_0; // Phrase one
    payload.bytes[2] = message_part_1; // Phrase two
    payload.bytes[3] = message_part_2; // Phrase three

    oled_ui.show_debug({display::DebugDirection::Sent},
                        0,
                        payload,
                        kPhraseDictionaryV1[message_part_0],
                        kPhraseDictionaryV1[message_part_1],
                        kPhraseDictionaryV1[message_part_2]);
    log_status_update_sending(message_part_0, message_part_1);

    mesh.send_payload(payload);
}

void Application::handle_received_status_update(const uint64_t origin_device_id, const protocol::Payload payload) {
    if (payload.bytes[1] >= kPhraseCountV1 || payload.bytes[2] >= kPhraseCountV1) {
        log_status_update_out_of_range(origin_device_id, payload.bytes[1], payload.bytes[2]);
        return;
    }

    oled_ui.show_debug({display::DebugDirection::Received},
                        origin_device_id,
                        payload,
                        kPhraseDictionaryV1[message_part_0],
                        kPhraseDictionaryV1[message_part_1],
                        kPhraseDictionaryV1[message_part_2]);
    log_status_update_received(origin_device_id, payload.bytes[1], payload.bytes[2]);
}

void Application::init_nvs() {
    esp_err_t status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS unusable (%s), erasing - stored identity is lost", esp_err_to_name(status));
        ESP_ERROR_CHECK(nvs_flash_erase());
        status = nvs_flash_init();
    }
    ESP_ERROR_CHECK(status);
}

void Application::init_i2c() {
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_cfg.glitch_ignore_cnt = 7;
    i2c_bus_cfg.i2c_port = I2C_NUM_0;
    i2c_bus_cfg.sda_io_num = GPIO_NUM_17;
    i2c_bus_cfg.scl_io_num = GPIO_NUM_18;
    i2c_bus_cfg.flags.enable_internal_pullup = true;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_handle));
}

void Application::init_btn() {
    ESP_LOGI(TAG, "Initialize button");
    btn_ctx.app_event_queue = app_queue_handle;
    button_cfg_t btn_cfg = BUTTON_CFG_DEFAULT(button_pin, handle_button_callback);
    btn_cfg.user_data = &btn_ctx;
    btn_cfg.hasPullup = true;
    ESP_ERROR_CHECK(button_service_init());
    ESP_ERROR_CHECK(button_init(&btn_cfg, &main_btn));
}

}
