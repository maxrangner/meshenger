#include "utils.h"

#include <cstdint>

namespace protocol::utils {

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
        buffer[i + 21] = static_cast<uint8_t>(packet.payload.bytes[i]);
    }
}

bool deserialize_packet(const uint8_t* buffer, size_t received_size, protocol::Packet& packet) {
    if (received_size != protocol::kSerializedPacketSize) {
        return false;
    }

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
    
    for (int i = 0; i < protocol::kPayloadSize; i++) {
        packet.payload.bytes[i] = static_cast<char>(buffer[i + 21]);
    }

    return true;
}

}
