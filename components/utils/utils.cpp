#include "utils.h"

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "esp_mac.h"

namespace utils {

void serialize_packet(const protocol::Packet& packet, uint8_t* buffer) {
    buffer[0] = packet.version;

    // group_id
    for (int i = 0; i < 8; i++) {
        buffer[i + 1] = static_cast<uint8_t>(packet.message_id.group_id >> ((7 - i) * 8));
    }

    // origin_device_id
    for (int i = 0; i < 8; i++) {
        buffer[i + 9] = static_cast<uint8_t>(packet.message_id.origin_device_id >> ((7 - i) * 8));
    }

    // message_num
    for (int i = 0; i < 4; i++) {
        buffer[i + 17] = static_cast<uint8_t>(packet.message_id.message_num >> ((3 - i) * 8));
    }

    for (int i = 0; i < protocol::kPayloadSize; i++) {
        buffer[i + 21] = static_cast<uint8_t>(packet.payload[i]);
    }
}

void deserialize_packet(const uint8_t* buffer, protocol::Packet& packet) {
    packet.version = static_cast<uint8_t>(buffer[0]);

    packet.message_id.group_id = 0;
    for (int i = 0; i < 8; i++) {
        packet.message_id.group_id = (packet.message_id.group_id << 8 | buffer[i + 1]);
    }

    packet.message_id.origin_device_id = 0;
    for (int i = 0; i < 8; i++) {
        packet.message_id.origin_device_id =
            (packet.message_id.origin_device_id << 8) | buffer[i + 9];
    }

    packet.message_id.message_num = 0;
    for (int i = 0; i < 4; i++) {
        packet.message_id.message_num =
            (packet.message_id.message_num << 8) | buffer[i + 17];
    }
    
    for (int i = 0; i < protocol::kPayloadSize - 1; i++) {
        packet.payload[i] = static_cast<char>(buffer[i + 21]);
    }
    packet.payload[protocol::kPayloadSize - 1] = '\0';
}

uint64_t get_mac_address() {
    uint8_t device_mac[6];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(device_mac));
    
    uint64_t converted_mac = 0;
    for (int i = 0; i < 6; i++) {
        converted_mac = (converted_mac << 8) | device_mac[i];
    }

    return converted_mac;
}

}
