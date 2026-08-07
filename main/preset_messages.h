#pragma once

#include <cstdint>

namespace app {

const uint8_t kNumPartsId = 2;

static const char *const message_parts[kNumPartsId] = {
    "hello",
    "world"
};

}
