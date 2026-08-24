#include "packet_codec.h"

#include <cstdint>

namespace protocol::codec {

void serialize_packet(const protocol::Packet& packet, uint8_t* buffer) {
    // protocol version. buffer[0]
    buffer[0] = packet.header.protocol_version;

    // group_id. buffer[1-8]
    for (int i = 0; i < 8; i++) {
        buffer[i + 1] = static_cast<uint8_t>(packet.header.message_id.group_id >> ((7 - i) * 8));
    }

    // origin_device_id. buffer[9-16]
    for (int i = 0; i < 8; i++) {
        buffer[i + 9] = static_cast<uint8_t>(packet.header.message_id.origin_device_id >> ((7 - i) * 8));
    }

    // message_num. buffer[17-20]
    for (int i = 0; i < 4; i++) {
        buffer[i + 17] = static_cast<uint8_t>(packet.header.message_id.sequence_num >> ((3 - i) * 8));
    }
    
    buffer[21] = packet.header.phrase_dictionary_version;
    buffer[22] = packet.header.hop_limit;

    // payload. buffer[23-26]
    for (int i = 0; i < protocol::kPayloadSizeBytes; i++) {
        buffer[i + 23] = static_cast<uint8_t>(packet.payload.bytes[i]);
    }
}

bool deserialize_packet(const uint8_t* buffer, size_t received_size, protocol::Packet& packet) {
    if (received_size != protocol::kSerializedPacketSize) {
        return false;
    }

    packet.header.protocol_version = static_cast<uint8_t>(buffer[0]);

    packet.header.message_id.group_id = 0;
    for (int i = 0; i < 8; i++) {
        packet.header.message_id.group_id = (packet.header.message_id.group_id << 8 | buffer[i + 1]);
    }

    packet.header.message_id.origin_device_id = 0;
    for (int i = 0; i < 8; i++) {
        packet.header.message_id.origin_device_id = (packet.header.message_id.origin_device_id << 8) | buffer[i + 9];
    }

    packet.header.message_id.sequence_num = 0;
    for (int i = 0; i < 4; i++) {
        packet.header.message_id.sequence_num = (packet.header.message_id.sequence_num << 8) | buffer[i + 17];
    }

    packet.header.phrase_dictionary_version = buffer[21];
    packet.header.hop_limit = buffer[22];
    
    for (int i = 0; i < protocol::kPayloadSizeBytes; i++) {
        packet.payload.bytes[i] = buffer[i + 23];
    }

    return true;
}

}
