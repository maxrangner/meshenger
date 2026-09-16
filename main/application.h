#pragma once

#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"
#include "node_state.h"
#include "radio_service.h"
#include "packet.h"
#include "mesh_service.h"
#include "button_driver.h"
#include "oled_display.h"
#include "oled_ui.h"

namespace app {

struct ButtonContext {
    QueueHandle_t app_event_queue;
};

class Application {
public:
    void init();
    void send_status_update();
    void handle_received_status_update(const uint64_t origin_device_id, const protocol::Payload payload);
private:
    void init_nvs();
    void init_i2c();
    void init_btn();
    static void app_task(void* pvParameters);
    void change_message();

    TaskHandle_t app_task_handle = nullptr;
    QueueHandle_t app_queue_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 0;

    i2c_master_bus_handle_t i2c_bus_handle = nullptr;
    i2c_master_bus_config_t i2c_bus_cfg{};

    display::OledDisplay oled_display;
    display::OledUi oled_ui{oled_display};
    
    mesh::MeshService mesh;

    gpio_num_t button_pin = GPIO_NUM_0;
    button_t main_btn;
    ButtonContext btn_ctx;

    uint8_t current_selected_mgs = 0;
};

}
