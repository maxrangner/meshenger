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
#include "epaper_display.h"

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
    void init_vext();
    void init_i2c();
    void init_spi();
    void init_btn();
    static void app_task(void* pvParameters);

    TaskHandle_t app_task_handle = nullptr;
    QueueHandle_t app_queue_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 0;

    // Vext powers both panels, so the rail belongs to the board rather than to one display.
    gpio_num_t vext_pin = GPIO_NUM_36;

    i2c_master_bus_handle_t i2c_bus_handle = nullptr;
    i2c_master_bus_config_t i2c_bus_cfg{};

    spi_host_device_t epaper_spi_host = SPI3_HOST;
    spi_bus_config_t epaper_spi_bus_cfg{};
    gpio_num_t epaper_sclk_pin = GPIO_NUM_33;
    gpio_num_t epaper_mosi_pin = GPIO_NUM_47;

    display::OledDisplay oled_display;
    display::OledUi oled_ui{oled_display};

    display::EpaperDisplay epaper_display;
    display::EpaperConfig epaper_cfg{
        .cs_pin = GPIO_NUM_48,
        .dc_pin = GPIO_NUM_4,
        .reset_pin = GPIO_NUM_6,
        .busy_pin = GPIO_NUM_3,
    };

    mesh::MeshService mesh;

    gpio_num_t button_pin = GPIO_NUM_0;
    button_t main_btn;
    ButtonContext btn_ctx;

    uint8_t message_part_0 = 0;
    uint8_t message_part_1 = 1;
    uint8_t message_part_2 = 2;
};

}
