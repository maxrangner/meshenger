#pragma once

#include <cstdint>

namespace mesh {

struct LocalNodeState {
    uint64_t group_id;
    uint64_t device_id;
    uint32_t next_sequence_num;
};

}