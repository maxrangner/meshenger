#include "radio_service.h"

#include "esp_log.h"

static const char *TAG = "RadioService";
volatile bool RadioService::packet_received_ = false;

RadioService::RadioService() : hal_(spi_sck_pin_, spi_miso_pin_, spi_mosi_pin_),
                               module_(&hal_, spi_cs_pin_, sx1262_dio1_pin_, sx1262_reset_pin_, sx1262_busy_pin_),
                               radio_(&module_) {

}

void IRAM_ATTR RadioService::onPacketReceived() {
    packet_received_ = true;
}

int RadioService::begin() {
    // Init Heltec v4.3.1 external front-end
    ESP_ERROR_CHECK(gpio_set_direction(VFEM_Ctrl_pin_, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PA_CSD_pin_, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PA_CTX_pin_, GPIO_MODE_OUTPUT));

    ESP_ERROR_CHECK(gpio_set_level(PA_CTX_pin_, 0)); // Safe non-TX state before RadioLib takes control
    ESP_ERROR_CHECK(gpio_set_level(VFEM_Ctrl_pin_, 1)); // Power/activation of front-end
    ESP_ERROR_CHECK(gpio_set_level(PA_CSD_pin_, 1)); // Activates KCT8103L

    // Set up the RF switch pins and mode table
    const uint32_t rf_switch_pins[Module::RFSWITCH_MAX_PINS] = {
        static_cast<uint32_t>(PA_CTX_pin_),
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
    ConfigLoRa_t config = {
        .frequency = 869.525,
        .bandwidth = 125.0,
        .spreadingFactor = spreading_factor_,
        .codingRate = 5,
        .syncWord = RADIOLIB_LORA_SYNC_WORD_PRIVATE,
        .power = radio_power_,
        .preambleLength = 8
    };

    radio_.setRfSwitchTable(rf_switch_pins, rf_switch_table);

    radio_.tcxoVoltage = 1.8f;
    int state = radio_.begin(config);
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "[SX1262] Initialization failed, code %d", state);
        return state;
    }

    radio_.setPacketReceivedAction(onPacketReceived);

    ESP_LOGI(TAG, "[SX1262] Initialized successfully");
    return RADIOLIB_ERR_NONE;
}

int RadioService::startReceive() {
    packet_received_ = false;
    const int state = radio_.startReceive();

    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "failed, code %d\n", state);
        return state;
    }

    return state;
}

void RadioService::stop() {
    // Stop implementation
}

void RadioService::send() {
    // Send implementation
}