#pragma once

#include <initializer_list>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"

namespace display {

// The panel is portrait: 122 pixels along the source lines and 250 gate lines.
// Rows are padded to whole bytes, so every row occupies 16 bytes and leaves 6 bits unused.
constexpr uint16_t kEpaperWidth = 122;
constexpr uint16_t kEpaperHeight = 250;
constexpr uint16_t kEpaperStride = 16;
constexpr uint16_t kEpaperBufferSize = kEpaperStride * kEpaperHeight;

enum class RefreshMode {
    Full,       // Flashes, clears ghosting, and stores the base image that partials are compared against
    Partial,    // No flash, but ghosts over time - follow every few partials with a full refresh
};

struct EpaperConfig {
    gpio_num_t cs_pin;
    gpio_num_t dc_pin;
    gpio_num_t reset_pin;
    gpio_num_t busy_pin;
};

class EpaperDisplay {
public:
    void init_epaper(const spi_host_device_t spi_host, const EpaperConfig& config);

    void clear();
    void display_test_pattern();
    void display_text(const uint16_t x, const uint16_t y, const char* text);
    void set_pixel(const uint16_t x, const uint16_t y, const bool black);
    bool refresh(const RefreshMode mode);
    void sleep();
private:
    bool init_panel();
    void init_panel_for_partial();
    void reset_panel(const uint32_t low_time_ms);
    void set_ram_area();
    void write_command(const uint8_t command, std::initializer_list<uint8_t> parameters = {});
    void write_framebuffer(const uint8_t ram_command);
    bool run_update_sequence(const uint8_t update_sequence);
    bool wait_until_idle();

    EpaperConfig epaper_cfg{};

    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_io_spi_config_t io_config{};

    uint8_t framebuffer[kEpaperBufferSize]{};
    bool panel_asleep = false;
    bool base_image_valid = false;
};

}
