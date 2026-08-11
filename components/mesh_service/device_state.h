#pragma once

#include <cstdint>

namespace mesh {

struct DeviceState {
    uint8_t protocol_version;
    uint8_t dictionary_version;
    uint64_t group_id;
    uint64_t device_id;
    uint32_t packet_count;
};

}