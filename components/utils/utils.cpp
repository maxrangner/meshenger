#include "utils.h"

#include <cstdint>

namespace utils {

void serialize_packet(const protocol::Packet& packet, uint8_t* buffer) {
    buffer[0] = packet.version;
    buffer[1] = packet.sender_id;
    // memcopy
    for (int i = 0; i < protocol::kPacketSize - 2; i++) {
        buffer[i + 2] = packet.payload[i];
    }
}

void deserialize_packet(const uint8_t* buffer, protocol::Packet& packet) {
    packet.version = buffer[0];
    packet.sender_id = buffer[1];
    // memcopy
    for (int i = 0; i < protocol::kPacketSize - 2; i++) {
        packet.payload[i] = buffer[i + 2];
    }
    packet.payload[protocol::kPacketSize - 3] = '\0';
}

}
