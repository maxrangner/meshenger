#include "utils.h"

#include <cstdint>

namespace utils {

void serializePacket(const protocol::Packet& packet, uint8_t* buffer) {
    buffer[0] = packet.version;
    buffer[1] = packet.senderId;
    // memcopy
    for (int i = 0; i < protocol::kPacketSize - 2; i++) {
        buffer[i + 2] = packet.payload[i];
    }
}

void deserializePacket(const uint8_t* buffer, protocol::Packet& packet) {
    packet.version = buffer[0];
    packet.senderId = buffer[1];
    // memcopy
    for (int i = 0; i < protocol::kPacketSize - 2; i++) {
        packet.payload[i] = buffer[i + 2];
    }
    packet.payload[protocol::kPacketSize - 3] = '\0';
}

}