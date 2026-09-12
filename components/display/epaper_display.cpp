#include "epaper_display.h"

#include <cinttypes>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "font_5x7.h"

constexpr char TAG[] = "epaper_display";

namespace display {

namespace {

// SSD1680 commands, named after the datasheet.
constexpr uint8_t kCmdDriverOutputControl = 0x01;
constexpr uint8_t kCmdDeepSleep = 0x10;
constexpr uint8_t kCmdDataEntryMode = 0x11;
constexpr uint8_t kCmdSoftwareReset = 0x12;
constexpr uint8_t kCmdTemperatureSensor = 0x18;
constexpr uint8_t kCmdActivateUpdate = 0x20;
constexpr uint8_t kCmdUpdateControl1 = 0x21;
constexpr uint8_t kCmdUpdateControl2 = 0x22;
constexpr uint8_t kCmdWriteRam = 0x24;
constexpr uint8_t kCmdWriteBaseRam = 0x26;
constexpr uint8_t kCmdBorderWaveform = 0x3C;
constexpr uint8_t kCmdRamXRange = 0x44;
constexpr uint8_t kCmdRamYRange = 0x45;
constexpr uint8_t kCmdRamXCounter = 0x4E;
constexpr uint8_t kCmdRamYCounter = 0x4F;

// Command parameters. The panel stores its own waveforms, so these only pick between them.
constexpr uint8_t kDataEntryIncrementXY = 0x03;
constexpr uint8_t kBorderFollowWaveform = 0x05;
constexpr uint8_t kBorderHighImpedance = 0x80;
constexpr uint8_t kTemperatureSensorInternal = 0x80;
constexpr uint8_t kDeepSleepRetainRam = 0x01;
constexpr uint8_t kUpdateSequenceFull = 0xF7;
constexpr uint8_t kUpdateSequencePartial = 0xFF;

// The last gate line addresses the bottom row of the panel.
constexpr uint8_t kLastGateLineLow = static_cast<uint8_t>((kEpaperHeight - 1) & 0xFF);
constexpr uint8_t kLastGateLineHigh = static_cast<uint8_t>((kEpaperHeight - 1) >> 8);
constexpr uint8_t kLastRamXByte = static_cast<uint8_t>(kEpaperStride - 1);

// A set bit is white on this panel, a cleared bit is black.
constexpr uint8_t kWhiteByte = 0xFF;
constexpr uint8_t kBlackByte = 0x00;

constexpr uint32_t kSpiClockHz = 10 * 1000 * 1000;  // The SSD1680 accepts up to 20 MHz
constexpr uint32_t kResetSettleMs = 20;
constexpr uint32_t kResetLowTimeMs = 2;
constexpr uint32_t kPartialResetLowTimeMs = 1;
constexpr uint32_t kDeepSleepSettleMs = 100;
constexpr uint32_t kBusyPollIntervalMs = 10;
constexpr uint32_t kBusyTimeoutMs = 10000;          // A full refresh takes about 4 s at room temperature

}

void EpaperDisplay::init_epaper(const spi_host_device_t spi_host, const EpaperConfig& config) {
    epaper_cfg = config;

    ESP_LOGI(TAG, "Configure reset and busy lines");
    ESP_ERROR_CHECK(gpio_set_direction(epaper_cfg.reset_pin, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(epaper_cfg.reset_pin, 1));
    ESP_ERROR_CHECK(gpio_set_direction(epaper_cfg.busy_pin, GPIO_MODE_INPUT));
    // The panel only drives busy once Vext is up, so hold the line at idle until it does.
    ESP_ERROR_CHECK(gpio_set_pull_mode(epaper_cfg.busy_pin, GPIO_PULLDOWN_ONLY));

    ESP_LOGI(TAG, "Install panel IO");
    io_config.cs_gpio_num = epaper_cfg.cs_pin;
    io_config.dc_gpio_num = epaper_cfg.dc_pin;
    io_config.spi_mode = 0;
    io_config.pclk_hz = kSpiClockHz;
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(spi_host, &io_config, &io_handle));

    ESP_LOGI(TAG, "Initialize SSD1680 panel");
    if (!init_panel()) {
        ESP_LOGE(TAG, "panel did not respond during init - the e-paper stays blank");
    }

    base_image_valid = false;

    clear();
}

void EpaperDisplay::clear() {
    for (int i = 0; i < kEpaperBufferSize; i++) {
        framebuffer[i] = kWhiteByte;
    }
}

void EpaperDisplay::display_test_pattern() {
    for (int i = 0; i < kEpaperBufferSize; i++) {
        framebuffer[i] = kBlackByte;
    }
}

void EpaperDisplay::display_text(const uint16_t x, const uint16_t y, const char* text) {
    if (text == nullptr || x > kEpaperWidth - kCharacterWidth || y >= kEpaperHeight) {
        return;
    }

    uint16_t character_x = x;

    while (*text != '\0' && character_x <= kEpaperWidth - kCharacterWidth) {
        uint8_t character = static_cast<uint8_t>(*text++);
        if (character < kFirstPrintableCharacter || character > kLastPrintableCharacter) {
            character = kFallbackCharacter;
        }

        const uint8_t* glyph = &kFont[(character - kFirstPrintableCharacter) * kGlyphWidth];
        for (uint8_t column = 0; column < kCharacterWidth; ++column) {
            for (uint8_t row = 0; row < kGlyphHeight && y + row < kEpaperHeight; ++row) {
                // The spacing column carries no glyph data, so it clears whatever it covers.
                const bool black = column < kGlyphWidth && (glyph[column] & (1U << row));
                set_pixel(character_x + column, y + row, black);
            }
        }

        character_x += kCharacterWidth;
    }
}

void EpaperDisplay::set_pixel(const uint16_t x, const uint16_t y, const bool black) {
    if (x >= kEpaperWidth || y >= kEpaperHeight) {
        return;
    }

    const uint16_t byte_index = y * kEpaperStride + (x / 8);
    const uint8_t bit = static_cast<uint8_t>(0x80 >> (x % 8));

    if (black) {
        framebuffer[byte_index] &= static_cast<uint8_t>(~bit);
    } else {
        framebuffer[byte_index] |= bit;
    }
}

bool EpaperDisplay::refresh(const RefreshMode mode) {
    // Deep sleep mode 1 keeps the panel RAM, so waking only costs a reset and a fresh init.
    if (panel_asleep && !init_panel()) {
        ESP_LOGE(TAG, "panel did not wake from deep sleep - the image is now stale");
        return false;
    }

    // A partial refresh is drawn against the base image, which only a full refresh can write.
    if (mode == RefreshMode::Partial && base_image_valid) {
        init_panel_for_partial();
        write_framebuffer(kCmdWriteRam);

        return run_update_sequence(kUpdateSequencePartial);
    }

    // A full refresh writes the base image too, so a partial refresh can follow it directly.
    write_framebuffer(kCmdWriteRam);
    write_framebuffer(kCmdWriteBaseRam);

    base_image_valid = run_update_sequence(kUpdateSequenceFull);

    return base_image_valid;
}

void EpaperDisplay::sleep() {
    write_command(kCmdDeepSleep, {kDeepSleepRetainRam});
    vTaskDelay(pdMS_TO_TICKS(kDeepSleepSettleMs));

    panel_asleep = true;
}

bool EpaperDisplay::init_panel() {
    reset_panel(kResetLowTimeMs);

    if (!wait_until_idle()) {
        return false;
    }

    write_command(kCmdSoftwareReset);

    if (!wait_until_idle()) {
        return false;
    }

    write_command(kCmdDriverOutputControl, {kLastGateLineLow, kLastGateLineHigh, 0x00});
    write_command(kCmdDataEntryMode, {kDataEntryIncrementXY});
    set_ram_area();
    write_command(kCmdBorderWaveform, {kBorderFollowWaveform});
    write_command(kCmdUpdateControl1, {0x00, 0x80});
    write_command(kCmdTemperatureSensor, {kTemperatureSensorInternal});

    panel_asleep = false;

    return wait_until_idle();
}

// A partial refresh reconfigures the panel without a software reset, which would discard the base image.
void EpaperDisplay::init_panel_for_partial() {
    reset_panel(kPartialResetLowTimeMs);

    write_command(kCmdBorderWaveform, {kBorderHighImpedance});
    write_command(kCmdDriverOutputControl, {kLastGateLineLow, kLastGateLineHigh, 0x00});
    write_command(kCmdDataEntryMode, {kDataEntryIncrementXY});
    set_ram_area();
}

void EpaperDisplay::reset_panel(const uint32_t low_time_ms) {
    ESP_ERROR_CHECK(gpio_set_level(epaper_cfg.reset_pin, 1));
    vTaskDelay(pdMS_TO_TICKS(kResetSettleMs));
    ESP_ERROR_CHECK(gpio_set_level(epaper_cfg.reset_pin, 0));
    vTaskDelay(pdMS_TO_TICKS(low_time_ms));
    ESP_ERROR_CHECK(gpio_set_level(epaper_cfg.reset_pin, 1));
    vTaskDelay(pdMS_TO_TICKS(kResetSettleMs));
}

void EpaperDisplay::set_ram_area() {
    write_command(kCmdRamXRange, {0x00, kLastRamXByte});
    write_command(kCmdRamYRange, {0x00, 0x00, kLastGateLineLow, kLastGateLineHigh});
    write_command(kCmdRamXCounter, {0x00});
    write_command(kCmdRamYCounter, {0x00, 0x00});
}

void EpaperDisplay::write_command(const uint8_t command, std::initializer_list<uint8_t> parameters) {
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, command, parameters.begin(), parameters.size()));
}

void EpaperDisplay::write_framebuffer(const uint8_t ram_command) {
    write_command(kCmdRamXCounter, {0x00});
    write_command(kCmdRamYCounter, {0x00, 0x00});

    // The pixel transfer runs in the background, but the next command waits for it to finish.
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_color(io_handle, ram_command, framebuffer, kEpaperBufferSize));
}

bool EpaperDisplay::run_update_sequence(const uint8_t update_sequence) {
    write_command(kCmdUpdateControl2, {update_sequence});
    write_command(kCmdActivateUpdate);

    return wait_until_idle();
}

bool EpaperDisplay::wait_until_idle() {
    // The busy line stays high for as long as the panel is working.
    for (uint32_t waited_ms = 0; waited_ms < kBusyTimeoutMs; waited_ms += kBusyPollIntervalMs) {
        if (gpio_get_level(epaper_cfg.busy_pin) == 0) {
            return true;
        }

        vTaskDelay(pdMS_TO_TICKS(kBusyPollIntervalMs));
    }

    ESP_LOGE(TAG, "busy line still held after %" PRIu32 " ms - panel is unresponsive", kBusyTimeoutMs);

    return false;
}

}
