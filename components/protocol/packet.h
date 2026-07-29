#pragma once

#include <cstdint>

namespace protocol {

constexpr uint8_t kPacketVersion = 1;
constexpr uint8_t kPacketSize = 61;
constexpr uint8_t kPayloadSize = 40;

struct Packet {
    uint8_t version;
    uint64_t group_id;
    uint64_t device_id;
    uint32_t message_id;
    char payload[kPayloadSize];
};

}
