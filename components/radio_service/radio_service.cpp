#include "radio_service.h"

#include "utils.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "app_message.h"

static const char *TAG = "RadioService";

namespace radio {

RadioService::RadioService() : hal(spi_sck_pin, spi_miso_pin, spi_mosi_pin),
                               module(&hal, spi_cs_pin, sx1262_dio1_pin, sx1262_reset_pin, sx1262_busy_pin),
                               radio(&module) {
}

void IRAM_ATTR RadioService::radio_event() {
    RadioCommand command;
    command.message = RadioCommandMessage::RADIO_EVENT;

    xQueueSendFromISR(radio_queue_handle, &command, nullptr);
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

int RadioService::init(QueueHandle_t app_queue) {
    ESP_LOGI(TAG, "Initializing RadioService...");

    int state = init_radio();
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }

    ESP_LOGI(TAG, "RadioService and [SX1262] Initialized successfully");

    radio_queue_handle = xQueueCreate(10, sizeof(radio::RadioCommand));
    app_queue_handle = app_queue;

    xTaskCreatePinnedToCore(
        radio_service_task,        // Function to implement the task
        "RadioServiceTask",        // Name of the task
        8192,                      // Stack size in bytes
        this,                      // Task input parameter
        1,                         // Priority of the task
        &radio_task_handle,        // Task handle.
        kTaskCore                  // Core where the task should run
    );

    radio.setDio1Action(radio_event);
    radio_state = RadioState::RECEIVING;

    return RADIOLIB_ERR_NONE;
}

void RadioService::radio_service_task(void* pvParameters) {
    auto* self = static_cast<RadioService*>(pvParameters);

    ESP_LOGI(TAG, "Running radio task on core %d", kTaskCore);

    RadioCommand incoming_command;
    // uint8_t buffer[protocol::kPacketSize] = {};
    // protocol::Packet packet = {};

    self->start_rx();

    while(true) {
        xQueueReceive(self->radio_queue_handle, &incoming_command, portMAX_DELAY);
        switch (incoming_command.message) {
            case RadioCommandMessage::SEND_PACKET:
                switch (self->radio_state) {
                    case RadioState::RECEIVING:
                        self->transmit(incoming_command.payload);
                        break;
                    default:
                        break;
                    }
                break;
            case RadioCommandMessage::RADIO_EVENT:
                switch (self->radio_state) {
                    case RadioState::RECEIVING:
                        self->read_new_packet();
                        break;
                    case RadioState::TRANSMITTING:
                        self->transmit_complete();
                        break;
                    }
                break;
        }
    }
}

void RadioService::start_rx() {
    ESP_LOGI(TAG, "Start RX");

    int state = radio.startReceive();

    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "Failed to start receiving, code %d", state);
    } else {
        ESP_LOGI(TAG, "Started receiving");
    }
}

void RadioService::read_new_packet() {
    ESP_LOGI(TAG, "Reading new packet.");

    uint8_t incoming_packet[protocol::kPacketSize];
    int state = radio.readData(incoming_packet, protocol::kPacketSize);

    if (state == RADIOLIB_ERR_NONE) {
        AppEvent event;
        event.message = AppEventMessage::MESSAGE_RECEIVED;
        memcpy(event.payload, &incoming_packet, protocol::kPacketSize);

        xQueueSend(app_queue_handle, &event, portMAX_DELAY);
        ESP_LOGI(TAG, "Packet received! Sending to AppController for processing...");
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
        ESP_LOGI(TAG, "Receive timeout");
    } else {
        ESP_LOGI(TAG, "Failed to start receiving, code %d", state);
    }
}

int RadioService::transmit(const uint8_t* serialized_packet) {
    ESP_LOGI(TAG, "TX");

    radio_state = RadioState::TRANSMITTING;

    radio.standby();

    int state = radio.startTransmit(serialized_packet, protocol::kPacketSize);
    if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Transmission started!");
    } else {
        radio_state = RadioState::RECEIVING; 
        radio.startReceive();
        ESP_LOGI(TAG, "Failed to start transmitting, code %d", state);
    }

    return state;
}

void RadioService::transmit_complete() {
    ESP_LOGI(TAG, "TX complete");
    radio.finishTransmit();
    radio_state = RadioState::RECEIVING;
    start_rx();
}

void RadioService::send_packet(const uint8_t* serialized_packet) {
    ESP_LOGI(TAG, "Send packet");

    RadioCommand command;
    command.message = RadioCommandMessage::SEND_PACKET;
    memcpy(command.payload, serialized_packet, protocol::kPacketSize);

    xQueueSend(radio_queue_handle, &command, portMAX_DELAY);
}

}
