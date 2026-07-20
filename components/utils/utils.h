#pragma once

#include "packet.h"

namespace utils {

void serialize_packet(const protocol::Packet& packet, uint8_t* buffer);
void deserialize_packet(const uint8_t* buffer, protocol::Packet& packet);

}
