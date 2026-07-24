#pragma once

#include <RadioLib.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "hal/ESP-IDF/EspHal.h"
#include "button_driver.h"

#include "packet.h"

enum class RadioServiceEvent {
    SEND_PACKET,
    RADIO_EVENT,
};

enum class RadioState {
    TRANSMITTING,
    RECEIVING
};

struct ButtonContext {
    QueueHandle_t queue;
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

    int8_t radio_power = -3;
    uint8_t spreading_factor = 9;

    const char kUnitId = 'A';

    TaskHandle_t radio_task_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 0;
    inline static QueueHandle_t radio_queue = nullptr;

    uint8_t transmit_interval_ms;

    gpio_num_t button_pin = GPIO_NUM_0;
    button_t main_btn;
    ButtonContext btn_ctx;

    EspHal hal;
    Module module;
    SX1262 radio;
    RadioState radio_state;

    static volatile bool packet_received;
    static void IRAM_ATTR radio_event();
public:
    RadioService();
    int init();
    static void radio_service_task(void* pvParameters);
    static void receive_task(void* pvParameters);
    void start_rx();
    void read_new_packet(uint8_t* buffer, size_t length, protocol::Packet& packet);
    int send(const uint8_t* buffer, size_t length);
};


