#pragma once

#include <cstdint>

namespace protocol {

constexpr uint8_t kPacketVersion = 1;
constexpr uint8_t kSerializedPacketSize = 25;
constexpr uint8_t kPayloadSize = 4;

struct MessageId {
    uint64_t group_id; // 8 bytes
    uint64_t origin_device_id; // 8 bytes
    uint32_t message_num; // 4 bytes
};

struct Payload {
    uint8_t bytes[kPayloadSize]{}; // 4 bytes
};

struct Packet { // 25 serialized bytes
    uint8_t version; // 1 byte
    MessageId message_id; // 20 bytes
    Payload payload; // 4 bytes
};

}
