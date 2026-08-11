#pragma once

#include <RadioLib.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "hal/ESP-IDF/EspHal.h"
#include "packet.h"

namespace radio {

const constexpr uint8_t kRadioPower = 14;
const constexpr uint8_t kSpreadingFactor = 9;

enum class RadioState {
    TRANSMITTING,
    RECEIVING,
    ERROR
};

enum class RadioResultType {
    FRAME_RECEIVED,
    TRANSMIT_COMPLETE,
    RADIO_BUSY,
    TRANSMITTING,
    ERROR,
    NONE
};

struct RadioResult {
    RadioResultType type;
    size_t received_size;
};

class RadioService {
private:
    gpio_num_t spi_cs_pin = static_cast<gpio_num_t>(8);
    gpio_num_t spi_sck_pin = static_cast<gpio_num_t>(9);
    gpio_num_t spi_mosi_pin = static_cast<gpio_num_t>(10);
    gpio_num_t spi_miso_pin = static_cast<gpio_num_t>(11);

    gpio_num_t sx1262_reset_pin = static_cast<gpio_num_t>(12);
    gpio_num_t sx1262_busy_pin = static_cast<gpio_num_t>(13);
    gpio_num_t sx1262_dio1_pin = static_cast<gpio_num_t>(14);

    gpio_num_t vfem_ctrl_pin = static_cast<gpio_num_t>(7);
    gpio_num_t pa_csd_pin = static_cast<gpio_num_t>(2);
    gpio_num_t pa_ctx_pin = static_cast<gpio_num_t>(5);

    int8_t radio_power = kRadioPower;
    uint8_t spreading_factor = kSpreadingFactor;
    uint8_t transmit_interval_ms;

    EspHal hal;
    Module module;
    SX1262 radio;
    RadioState radio_state;
    
    void irq_event();
    int init_radio();
    bool read_received_frame(uint8_t* receive_buffer);
    RadioResult handle_receive_complete(uint8_t* receive_buffer);
    RadioResult handle_transmit_complete();
    bool is_rx_done(const uint32_t &irq_flags);
    bool is_tx_done(const uint32_t &irq_flags);
public:
    RadioService();
    int init(void (*irq_callback)());
    bool start_rx();
    RadioResult handle_irq(uint8_t* receive_buffer);
    RadioResultType transmit(const uint8_t* serialized_packet);
};

}

