#pragma once

#include <RadioLib.h>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "hal/ESP-IDF/EspHal.h"


class RadioService {
private:
    gpio_num_t spi_cs_pin_ = static_cast<gpio_num_t>(8);
    gpio_num_t spi_sck_pin_ = static_cast<gpio_num_t>(9);
    gpio_num_t spi_mosi_pin_ = static_cast<gpio_num_t>(10);
    gpio_num_t spi_miso_pin_ = static_cast<gpio_num_t>(11);

    gpio_num_t sx1262_reset_pin_ = static_cast<gpio_num_t>(12);
    gpio_num_t sx1262_busy_pin_ = static_cast<gpio_num_t>(13);
    gpio_num_t sx1262_dio1_pin_ = static_cast<gpio_num_t>(14);

    gpio_num_t VFEM_Ctrl_pin_ = static_cast<gpio_num_t>(7);
    gpio_num_t PA_CSD_pin_ = static_cast<gpio_num_t>(2);
    gpio_num_t PA_CTX_pin_ = static_cast<gpio_num_t>(5);

    int8_t radio_power_ = -3;
    uint8_t spreading_factor_ = 9;

    EspHal hal_;
    Module module_;
    SX1262 radio_;

    static volatile bool packet_received_;
    static void IRAM_ATTR onPacketReceived();
    public:
    RadioService();
    
    int begin();
    void stop();
    void send();
    int startReceive();
};


