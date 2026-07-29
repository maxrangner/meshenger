#include "utils.h"

#include <cstdint>

namespace utils {

void serialize_packet(const protocol::Packet& packet, uint8_t* buffer) {
    buffer[0] = packet.version;
    buffer[1] = packet.device_id;
    // memcopy
    for (int i = 0; i < protocol::kPacketSize - 2; i++) {
        buffer[i + 2] = static_cast<uint8_t>(packet.payload[i]);
    }
}

void deserialize_packet(const uint8_t* buffer, protocol::Packet& packet) {
    packet.version = static_cast<uint8_t>(buffer[0]);
    packet.device_id = static_cast<uint8_t>(buffer[1]);
    // memcopy
    for (int i = 0; i < protocol::kPacketSize - 2; i++) {
        packet.payload[i] = static_cast<char>(buffer[i + 2]);
    }
    packet.payload[protocol::kPacketSize - 3] = '\0';
}

uint64_t mac_converter(const uint8_t* mac) {
    uint64_t converted_mac = 0;
    for (int i = 0; i < 6; i++) {
        converted_mac = (converted_mac << 8) | mac[i];
    }
    return converted_mac;
}

}
