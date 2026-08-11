#include "radio_service.h"

#include <cstring>
#include "sdkconfig.h"
#include "esp_log.h"
#include "app_message.h"

static const char *TAG = "RadioService";

namespace radio {

RadioService::RadioService() : hal(spi_sck_pin, spi_miso_pin, spi_mosi_pin),
                               module(&hal, spi_cs_pin, sx1262_dio1_pin, sx1262_reset_pin, sx1262_busy_pin),
                               radio(&module) {
}

int RadioService::init_radio() {
    // Init Heltec v4.3.1 external front-end
    ESP_ERROR_CHECK(gpio_set_direction(vfem_ctrl_pin, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pa_csd_pin, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pa_ctx_pin, GPIO_MODE_OUTPUT));

    ESP_ERROR_CHECK(gpio_set_level(pa_ctx_pin, 0)); // Safe non-TX state before RadioLib takes control
    ESP_ERROR_CHECK(gpio_set_level(vfem_ctrl_pin, 1)); // Power/activation of front-end
    ESP_ERROR_CHECK(gpio_set_level(pa_csd_pin, 1)); // Activates KCT8103L

    // Set up the RF switch pins and mode table
    const uint32_t rf_switch_pins[Module::RFSWITCH_MAX_PINS] = {
        static_cast<uint32_t>(pa_ctx_pin),
        RADIOLIB_NC,
        RADIOLIB_NC,
        RADIOLIB_NC,
        RADIOLIB_NC
    };

    static const Module::RfSwitchMode_t rf_switch_table[] = {
        {Module::MODE_IDLE, {0}},
        {Module::MODE_RX,   {0}},
        {Module::MODE_TX,   {1}},
        END_OF_MODE_TABLE
    };

    ESP_LOGI(TAG, "[SX1262] Initializing ... ");

    radio.setRfSwitchTable(rf_switch_pins, rf_switch_table);

    radio.tcxoVoltage = 1.8f;

    ConfigLoRa_t radio_config_t = {
        .frequency = 869.525,
        .bandwidth = 125.0,
        .spreadingFactor = spreading_factor,
        .codingRate = 5,
        .syncWord = RADIOLIB_LORA_SYNC_WORD_PRIVATE,
        .power = radio_power,
        .preambleLength = 8
    };
    
    int state = radio.begin(radio_config_t);
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "[SX1262] Initialization failed, code %d", state);
        return state;
    }

    return state;
}

int RadioService::init(void (*irq_callback)()) {
    ESP_LOGI(TAG, "Initializing RadioService...");

    int state = init_radio();
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }

    ESP_LOGI(TAG, "RadioService and [SX1262] Initialized successfully");

    radio.setDio1Action(irq_callback);
    radio_state = RadioState::RECEIVING;

    return RADIOLIB_ERR_NONE;
}

RadioResult RadioService::handle_irq(uint8_t* receive_buffer) {
    const uint32_t irq_flags = radio.getIrqFlags();

    if (is_rx_done(irq_flags)) {
        return handle_receive_complete(receive_buffer);
    } else if (is_tx_done(irq_flags)) {
        return handle_transmit_complete();
    } else {
        ESP_LOGI(TAG, "Stale or unexpected IRQ flags: 0x%08lx", static_cast<unsigned long>(irq_flags));

        if (irq_flags != 0) {
            radio.clearIrqFlags(irq_flags);
        }
        return {RadioResultType::ERROR, 0};
    }
}

bool RadioService::is_rx_done(const uint32_t &irq_flags) {
    return ((irq_flags & RADIOLIB_SX126X_IRQ_RX_DONE) && radio_state == RadioState::RECEIVING);
}

bool RadioService::is_tx_done(const uint32_t &irq_flags) {
    return ((irq_flags & RADIOLIB_SX126X_IRQ_TX_DONE) && radio_state == RadioState::TRANSMITTING);
}

bool RadioService::start_rx() {
    ESP_LOGI(TAG, "Start RX");

    int status = radio.startReceive();

    if (status != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "Failed to start receiving, code %d", status);
        return false;
    } else {
        ESP_LOGI(TAG, "Started receiving");
        return true;
    }
}

bool RadioService::read_received_frame(uint8_t* receive_buffer) {
    ESP_LOGI(TAG, "Reading new packet.");

    memset(receive_buffer, 0, protocol::kSerializedPacketSize);
    int state = radio.readData(receive_buffer, protocol::kSerializedPacketSize);

    // Handle error
    if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Packet received! Sending to AppController for processing...");
        return true;
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
        ESP_LOGE(TAG, "Receive timeout");
    } else {
        ESP_LOGE(TAG, "Failed to start receiving, code %d", state);
    }
    return false;
}

RadioResultType RadioService::transmit(const uint8_t* serialized_packet) {
    ESP_LOGI(TAG, "TX");
    
    if (radio_state != RadioState::RECEIVING) {
        return RadioResultType::RADIO_BUSY;
    }
    
    radio.standby();
    
    radio_state = RadioState::TRANSMITTING;
    int state = radio.startTransmit(serialized_packet, protocol::kSerializedPacketSize);
    if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Transmission started!");

        return RadioResultType::TRANSMITTING;
    } else {
        radio_state = RadioState::RECEIVING; 
        radio.startReceive();
        ESP_LOGI(TAG, "Failed to start transmitting, code %d", state);

        return RadioResultType::ERROR;
    }
}

RadioResult RadioService::handle_receive_complete(uint8_t* receive_buffer) {
    const size_t received_length = radio.getPacketLength();

    if (read_received_frame(receive_buffer)) {
        return {RadioResultType::FRAME_RECEIVED, received_length};
    } else {
        return {RadioResultType::ERROR, 0};
    }
}

RadioResult RadioService::handle_transmit_complete() {
    ESP_LOGI(TAG, "TX complete");
    if (radio.finishTransmit() != RADIOLIB_ERR_NONE) {
        radio_state = RadioState::ERROR;
        return {RadioResultType::ERROR, 0};
    }
    if (!start_rx()) {
        radio_state = RadioState::ERROR;
        return {RadioResultType::ERROR, 0};
    }
    radio_state = RadioState::RECEIVING;
    return {RadioResultType::TRANSMIT_COMPLETE, 0};
}

}
