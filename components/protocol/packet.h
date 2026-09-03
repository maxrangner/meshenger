#pragma once

#include <cstdint>

namespace protocol {

constexpr uint8_t kCurrentProtocolVersion = 1;
constexpr uint8_t kCurrentPhraseDictionaryVersion = 1;
constexpr uint8_t kSerializedPacketSize = 27;
constexpr uint8_t kPayloadSizeBytes = 4;
constexpr uint8_t kHopLimitDefault = 3;

// Bytes noted is serialized bytes

struct MessageId {
    uint64_t group_id; // 8 bytes
    uint64_t origin_device_id; // 8 bytes
    uint32_t sequence_num; // 4 bytes
};

struct Header {
    uint8_t protocol_version; // 1 byte
    MessageId message_id; // 20 bytes
    uint8_t phrase_dictionary_version; // 1 byte
    uint8_t hop_limit = kHopLimitDefault; // 1 byte
};

struct Payload {
    uint8_t bytes[kPayloadSizeBytes]{}; // 4 bytes
};

struct Packet { // 27 bytes
    Header header; // 23 bytes
    Payload payload; // 4 bytes
};

}
