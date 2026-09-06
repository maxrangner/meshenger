#include "oled_display.h"

#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

constexpr char TAG[] = "oled_display"; 

namespace display {

uint8_t OledDisplay::oled_buffer[kOledBufferSize]{};

void OledDisplay::init_oled(i2c_master_bus_handle_t i2c_bus) {
    ESP_LOGI(TAG, "Enable LCD power");
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_36, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_36, 0));

    vTaskDelay(pdMS_TO_TICKS(30));

    ESP_LOGI(TAG, "Install panel IO");
    io_config.dev_addr = 0x3C;
    io_config.scl_speed_hz = (400 * 1000);
    io_config.control_phase_bytes = 1;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    io_config.dc_bit_offset = 6;

    i2c_bus_handle = i2c_bus;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    panel_config.bits_per_pixel = 1;
    panel_config.reset_gpio_num = GPIO_NUM_21;

    ssd1306_config.height = 64;

    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}

void OledDisplay::display_test_pattern() {
    for (int i = 0; i < kOledBufferSize; i++) {
        oled_buffer[i] = 0xFF;
    }

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 64, oled_buffer));
}

void OledDisplay::display_text(uint8_t x, uint8_t y, const char* text) {

}

}