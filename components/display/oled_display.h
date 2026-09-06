#pragma once

#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_io.h"

namespace display {

constexpr uint16_t kOledBufferSize = 1024;

class OledDisplay {
public:
    void init_oled(i2c_master_bus_handle_t i2c_bus);
    void display_test_pattern();
    void display_text(uint8_t x, uint8_t y, const char* text);
private:
    static uint8_t oled_buffer[kOledBufferSize];

    i2c_master_bus_handle_t i2c_bus_handle = nullptr;

    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_io_i2c_config_t io_config{};

    esp_lcd_panel_handle_t panel_handle = nullptr;
    esp_lcd_panel_dev_config_t panel_config{};

    esp_lcd_panel_ssd1306_config_t ssd1306_config{};
};

}