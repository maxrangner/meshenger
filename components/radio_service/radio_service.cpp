#include "radio_service.h"

#include "packet.h"
#include "utils.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "RadioService";

RadioService::RadioService() : hal(spi_sck_pin, spi_miso_pin, spi_mosi_pin),
                               module(&hal, spi_cs_pin, sx1262_dio1_pin, sx1262_reset_pin, sx1262_busy_pin),
                               radio(&module) {
}

void IRAM_ATTR RadioService::radio_event() {
    RadioServiceEvent radio_event = RadioServiceEvent::RADIO_EVENT;
    xQueueSendFromISR(radio_queue, &radio_event, nullptr);
}

static void handle_button_callback(button_event_t event, gpio_num_t gpio_num, void *user_data) {
    auto* context = static_cast<ButtonContext*>(user_data);
    RadioServiceEvent button_event = RadioServiceEvent::SEND_PACKET;
    xQueueSend(context->queue, &button_event, 0);
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

    ESP_LOGI(TAG, "RadioService and [SX1262] Initialized successfully");

    radio_queue = xQueueCreate(10, sizeof(RadioServiceEvent));
    xTaskCreatePinnedToCore(       // RadioService Task
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

    button_cfg_t btn_cfg = BUTTON_CFG_DEFAULT(button_pin, handle_button_callback);
    btn_ctx.queue = radio_queue;
    btn_cfg.user_data = &btn_ctx;
    btn_cfg.hasPullup = true;

    ESP_ERROR_CHECK(button_service_init());
    ESP_ERROR_CHECK(button_init(&btn_cfg, &main_btn));

    return RADIOLIB_ERR_NONE;
}

void RadioService::radio_service_task(void* pvParameters) {
    auto* self = static_cast<RadioService*>(pvParameters);

    ESP_LOGI(TAG, "Running radio task");

    RadioServiceEvent event;
    uint8_t buffer[protocol::kPacketSize] = {};
    protocol::Packet packet = {};

    #if CONFIG_MESHENGER_DEVICE_ID_A
        constexpr char kUnitId = 'A';
    #elif CONFIG_MESHENGER_DEVICE_ID_B
        constexpr char kUnitId = 'B';
    #endif

    char message[32] = {};
    uint16_t num_packets = 0;

    self->start_rx();

    while(true) {
        xQueueReceive(self->radio_queue, &event, portMAX_DELAY);
        switch (event) {
            case RadioServiceEvent::SEND_PACKET:
                switch (self->radio_state) {
                    case RadioState::RECEIVING:
                        snprintf(message, sizeof(message), "Unit %c --- Packet #%d", kUnitId, num_packets++);
                        memcpy(packet.payload, message, strlen(message) + 1);
                        utils::serialize_packet(packet, buffer);
                        self->radio_state = RadioState::TRANSMITTING;
                        self->send(buffer, protocol::kPacketSize);
                        break;
                    default:
                        break;
                    }
                break;
            case RadioServiceEvent::RADIO_EVENT:
                switch (self->radio_state) {
                    case RadioState::RECEIVING:
                        self->read_new_packet(buffer, protocol::kPacketSize, packet);
                        break;
                    case RadioState::TRANSMITTING:
                        self->radio.finishTransmit();
                        self->radio_state = RadioState::RECEIVING;
                        self->start_rx();
                        break;
                    }
                break;
        }
    }
}

void RadioService::start_rx() {
    int state = radio.startReceive();

    if (state != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "Failed to start receiving, code %d", state);
    } else {
        ESP_LOGI(TAG, "Started receiving");
    }
}

void RadioService::read_new_packet(uint8_t* buffer, size_t capacity, protocol::Packet& packet) {
    int state = radio.readData(buffer, capacity);

    if (state == RADIOLIB_ERR_NONE) {
        utils::deserialize_packet(reinterpret_cast<const uint8_t*>(buffer), packet);

        ESP_LOGI(TAG, "Packet received!     Version: %d, Sender ID: %d, Message: %s", packet.version, packet.sender_id, packet.payload);
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
        ESP_LOGI(TAG, "Receive timeout");
    } else {
        ESP_LOGI(TAG, "Failed to start receiving, code %d", state);
    }
}

int RadioService::send(const uint8_t* buffer, size_t length) {
    radio.standby();
    int state = radio.startTransmit(buffer, length);

    if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Transmission started!");
    } else {
        radio_state = RadioState::RECEIVING; 
        radio.startReceive();
        ESP_LOGI(TAG, "Failed to start transmitting, code %d", state);
    }

    return state;
}
