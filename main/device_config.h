#pragma once

#include <cstdint>

namespace device {

struct DeviceConfig {
    uint64_t group_id;
    uint64_t origin_device_id;
};

}