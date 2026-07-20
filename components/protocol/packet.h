#pragma once

#include <cstdint>

namespace protocol {

constexpr uint8_t kPacketSize = 34;

struct Packet {
    uint8_t version;
    uint8_t sender_id;
    char payload[32];
};

}
