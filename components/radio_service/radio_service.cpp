#include "radio_service.h"

#include <cstring>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "app_event.h"

constexpr char TAG[] = "radio";

namespace radio {

namespace {

void log_radio_ready(const ConfigLoRa_t& config) {
    ESP_LOGI(TAG,
             "\n"
             "┌─ radio ────────────────────────────────\n"
             "│ chip     SX1262 + external front-end\n"
             "│ channel  %.3f MHz · BW %.1f kHz\n"
             "│ modem    SF%u · CR 4/%u · sync 0x%02X · preamble %u\n"
             "│ power    %d dBm\n"
             "└─ radio ready",
             config.frequency,
             config.bandwidth,
             config.spreadingFactor,
             config.codingRate,
             config.syncWord,
             config.preambleLength,
             config.power);
}

}

RadioService::RadioService() : hal(spi_sck_pin, spi_miso_pin, spi_mosi_pin),
                               module(&hal, spi_cs_pin, sx1262_dio1_pin, sx1262_reset_pin, sx1262_busy_pin),
                               radio(&module) {
}

int RadioService::init_radio() {
    // Init Heltec v4.3.1 external front-end
    ESP_ERROR_CHECK(gpio_set_direction(vfem_ctrl_pin, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pa_csd_pin, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(pa_ctx_pin, GPIO_MODE_OUTPUT));

    ESP_ERROR_CHECK(gpio_set_level(pa_ctx_pin, 0)); // Safe non-TX result_code before RadioLib takes control
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

    int status = radio.begin(radio_config_t);
    if (status != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "SX1262 begin() failed, code %d", status);
        return status;
    }

    log_radio_ready(radio_config_t);

    return status;
}

int RadioService::init(void (*irq_callback)()) {
    int status = init_radio();
    if (status != RADIOLIB_ERR_NONE) {
        return status;
    }

    radio.setDio1Action(irq_callback);
    radio_state = RadioState::Receiving;

    return RADIOLIB_ERR_NONE;
}

RadioResult RadioService::handle_irq(uint8_t* receive_buffer) {
    const uint32_t irq_flags = radio.getIrqFlags();

    if (is_rx_done(irq_flags)) {
        return handle_receive_complete(receive_buffer);
    } else if (is_tx_done(irq_flags)) {
        return handle_transmit_complete();
    } else {
        ESP_LOGW(TAG, "unexpected IRQ 0x%08lx while %s, ignored",
                 static_cast<unsigned long>(irq_flags),
                 radio_state == RadioState::Receiving ? "receiving"
                     : radio_state == RadioState::Transmitting ? "transmitting" : "in error state");

        if (irq_flags != 0) {
            radio.clearIrqFlags(irq_flags);
        }
        return {RadioResultType::Error, 0, 0.0f, 0.0f};
    }
}

bool RadioService::is_rx_done(const uint32_t &irq_flags) {
    return ((irq_flags & RADIOLIB_SX126X_IRQ_RX_DONE) && radio_state == RadioState::Receiving);
}

bool RadioService::is_tx_done(const uint32_t &irq_flags) {
    return ((irq_flags & RADIOLIB_SX126X_IRQ_TX_DONE) && radio_state == RadioState::Transmitting);
}

bool RadioService::start_rx() {
    int status = radio.startReceive();

    if (status != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "startReceive() failed, code %d - radio is now deaf", status);
        return false;
    }

    ESP_LOGD(TAG, "listening");
    return true;
}

bool RadioService::read_received_frame(uint8_t* receive_buffer) {
    memset(receive_buffer, 0, protocol::kSerializedPacketSize);
    int status = radio.readData(receive_buffer, protocol::kSerializedPacketSize);

    if (status == RADIOLIB_ERR_NONE) {
        return true;
    }

    if (status == RADIOLIB_ERR_CRC_MISMATCH) {
        ESP_LOGW(TAG, "frame dropped: CRC mismatch, corrupted on air");
    } else if (status == RADIOLIB_ERR_RX_TIMEOUT) {
        ESP_LOGW(TAG, "frame dropped: receive timeout");
    } else {
        ESP_LOGE(TAG, "readData() failed, code %d", status);
    }
    return false;
}

RadioResultType RadioService::transmit(const uint8_t* serialized_packet) {
    if (radio_state != RadioState::Receiving) {
        ESP_LOGW(TAG, "transmit rejected: radio is %s",
                 radio_state == RadioState::Transmitting ? "already transmitting" : "in error state");
        return RadioResultType::RadioBusy;
    }

    radio.standby();

    radio_state = RadioState::Transmitting;
    transmit_start_us = esp_timer_get_time();

    int status = radio.startTransmit(serialized_packet, protocol::kSerializedPacketSize);
    if (status == RADIOLIB_ERR_NONE) {
        ESP_LOGD(TAG, "transmitting %u bytes", protocol::kSerializedPacketSize);
        return RadioResultType::TransmissionStarted;
    }

    radio_state = RadioState::Receiving;
    radio.startReceive();
    ESP_LOGE(TAG, "startTransmit() failed, code %d - packet not sent", status);

    return RadioResultType::Error;
}

RadioResult RadioService::handle_receive_complete(uint8_t* receive_buffer) {
    const size_t received_size = radio.getPacketLength();
    const float rssi_dbm = radio.getRSSI();
    const float snr_db = radio.getSNR();

    if (!read_received_frame(receive_buffer)) {
        return {RadioResultType::Error, 0, rssi_dbm, snr_db};
    }

    return {RadioResultType::FrameReceived, received_size, rssi_dbm, snr_db};
}

RadioResult RadioService::handle_transmit_complete() {
    const int64_t air_time_ms = (esp_timer_get_time() - transmit_start_us) / 1000;

    int status = radio.finishTransmit();
    if (status != RADIOLIB_ERR_NONE) {
        radio_state = RadioState::Error;
        ESP_LOGE(TAG, "finishTransmit() failed, code %d - radio left in error state", status);
        return {RadioResultType::Error, 0, 0.0f, 0.0f};
    }
    if (!start_rx()) {
        radio_state = RadioState::Error;
        return {RadioResultType::Error, 0, 0.0f, 0.0f};
    }
    radio_state = RadioState::Receiving;

    ESP_LOGD(TAG, "transmit done in %lld ms, listening again", air_time_ms);
    return {RadioResultType::TransmissionComplete, 0, 0.0f, 0.0f};
}

}
