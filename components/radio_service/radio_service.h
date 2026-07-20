#pragma once

#include <RadioLib.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "hal/ESP-IDF/EspHal.h"

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

    TaskHandle_t receive_task_handle = nullptr;
    TaskHandle_t transmit_task_handle = nullptr;
    static constexpr BaseType_t kTaskCore = 0;

    uint8_t transmit_interval_ms;

    EspHal hal;
    Module module;
    SX1262 radio;

    static volatile bool packet_received;
    static void IRAM_ATTR on_packet_received();
public:
    RadioService();
    int init();
    void start(bool is_sender);
    static void transmit_task(void* pvParameters);
    static void receive_task(void* pvParameters);
    void stop();
    int receive(uint8_t* buffer, size_t length);
    int send(const uint8_t* buffer, size_t length);
};


