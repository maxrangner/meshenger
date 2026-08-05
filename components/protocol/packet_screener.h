#pragma once

#include "packet.h"

namespace protocol {

bool is_packet_for_group(uint64_t group_id, const Packet& packet);

}