#include "radio_service.h"

#include "packet.h"
#include "utils.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "RadioService";
volatile bool RadioService::packet_received = false;

RadioService::RadioService() : hal(spi_sck_pin, spi_miso_pin, spi_mosi_pin),
                               module(&hal, spi_cs_pin, sx1262_dio1_pin, sx1262_reset_pin, sx1262_busy_pin),
                               radio(&module) {
}

void IRAM_ATTR RadioService::on_packet_received() {
    packet_received = true;
}

int RadioService::init() {
    ESP_LOGI(TAG, "Initializing RadioService...");

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
    ConfigLoRa_t radio_config_t = {
        .frequency = 869.525,
        .bandwidth = 125.0,
        .spreadingFactor = spreading_factor,
        .codingRate = 5,
        .syncWord = RADIOLIB_LORA_SYNC_WORD_PRIVATE,
        .power = radio_power,
        .preambleLength = 8
    };

    radio.setRfSwitchTable(rf_switch_pins, rf_switch_table);

    radio.tcxoVoltage = 1.8f;
    int state = radio.begin(radio_config_t);
    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "[SX1262] Initialization failed, code %d", state);
        return state;
    }

    radio.setPacketReceivedAction(on_packet_received);

    ESP_LOGI(TAG, "RadioService and [SX1262] Initialized successfully");
    return RADIOLIB_ERR_NONE;
}

void RadioService::start(bool is_transmitter) {
    if (is_transmitter) {
        xTaskCreatePinnedToCore(       // Sender Task
            transmit_task,             // Function to implement the task
            "TransmitTask",            // Name of the task
            8192,                      // Stack size in bytes
            this,                      // Task input parameter
            1,                         // Priority of the task
            &transmit_task_handle,     // Task handle.
            kTaskCore                  // Core where the task should run
        );
    } else {
        xTaskCreatePinnedToCore(       // Receiver Task
            receive_task,              // Function to implement the task
            "ReceiveTask",             // Name of the task
            8192,                      // Stack size in bytes
            this,                      // Task input parameter
            1,                         // Priority of the task
            &receive_task_handle,      // Task handle.
            kTaskCore                  // Core where the task should run
        );
    }
}

void RadioService::transmit_task(void* pvParameters) {
    auto* self = static_cast<RadioService*>(pvParameters);

    ESP_LOGI(TAG, "Running transmit task");

    int state;
    protocol::Packet packet = {};
    packet.version = 1;
    packet.sender_id = 1;
    uint8_t buffer[protocol::kPacketSize] = {};

    int num_packets = 0;
    char message[32] = {};
    
    while(true) {
        snprintf(message, sizeof(message), "Send packet %d", num_packets++);
        memcpy(packet.payload, message, strlen(message) + 1);
        ESP_LOGI(TAG, "Preparing to send packet with message: %s", message);

        utils::serialize_packet(packet, buffer);
        state = self->send(buffer, protocol::kPacketSize);

        if (state != RADIOLIB_ERR_NONE) {
            ESP_LOGI(TAG, "Failed to start sending, code %d", state);
        }
        ESP_LOGI(TAG, "Packet sent successfully");

        vTaskDelay(pdMS_TO_TICKS(CONFIG_MESHENGER_SEND_INTERVAL_MS));
    }
}

void RadioService::receive_task(void* pvParameters) {
    auto* self = static_cast<RadioService*>(pvParameters);

    ESP_LOGI(TAG, "Calling receiver task");

    int state;
    uint8_t buffer[protocol::kPacketSize] = {};
    protocol::Packet packet;

    while(true) {
        memset(buffer, 0, sizeof(buffer));

        state = self->receive(buffer, protocol::kPacketSize);

        if (state == RADIOLIB_ERR_NONE) {
            utils::deserialize_packet(reinterpret_cast<const uint8_t*>(buffer), packet);

            ESP_LOGI(TAG, "Packet received!     Version: %d, Sender ID: %d, Message: %s", packet.version, packet.sender_id, packet.payload);
        } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
            ESP_LOGI(TAG, "Receive timeout");
        } else {
            ESP_LOGI(TAG, "Failed to start receiving, code %d", state);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void RadioService::stop() {
    // Stop implementation
}

int RadioService::receive(uint8_t* buffer, size_t capacity) {
    // packet_received = false;

    int state = radio.receive(buffer, capacity);

    return state;
}

int RadioService::send(const uint8_t* buffer, size_t length) {
    int state = radio.transmit(buffer, length);

    return state;
}
