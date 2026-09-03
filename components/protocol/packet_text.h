#pragma once

#include <cinttypes>
#include <cstdint>
#include <cstdio>

#include "packet.h"

namespace protocol::format {

struct IdText {
    char chars[19];
};

struct PayloadText {
    char chars[(3 * kPayloadSizeBytes) + 1];
};

inline IdText device_id(const uint64_t id) {
    IdText text{};
    if (id <= 0xFFFFFFFFFFFFull) {
        snprintf(text.chars, sizeof(text.chars), "%02X:%02X:%02X:%02X:%02X:%02X",
                 static_cast<unsigned>((id >> 40) & 0xFF),
                 static_cast<unsigned>((id >> 32) & 0xFF),
                 static_cast<unsigned>((id >> 24) & 0xFF),
                 static_cast<unsigned>((id >> 16) & 0xFF),
                 static_cast<unsigned>((id >> 8) & 0xFF),
                 static_cast<unsigned>(id & 0xFF));
    } else {
        snprintf(text.chars, sizeof(text.chars), "0x%016" PRIX64, id);
    }
    return text;
}

inline IdText group_id(const uint64_t id) {
    IdText text{};
    if (id <= 0xFFFFull) {
        snprintf(text.chars, sizeof(text.chars), "%" PRIu64, id);
    } else {
        snprintf(text.chars, sizeof(text.chars), "0x%016" PRIX64, id);
    }
    return text;
}

inline PayloadText payload(const Payload& payload) {
    PayloadText text{};
    for (uint8_t i = 0; i < kPayloadSizeBytes; i++) {
        snprintf(text.chars + (i * 3), 4, "%02X ", payload.bytes[i]);
    }
    text.chars[(3 * kPayloadSizeBytes) - 1] = '\0';
    return text;
}

}
