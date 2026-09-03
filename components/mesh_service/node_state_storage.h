#pragma once

#include "node_state.h"

namespace mesh {

bool load_node_state(LocalNodeState* state);
bool save_node_state(const LocalNodeState& state);
bool save_sequence_num(const uint32_t seq_num);

}
