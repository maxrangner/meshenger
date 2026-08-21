#pragma once

#include "packet.h"

namespace protocol::utils {

void serialize_packet(const protocol::Packet& packet, uint8_t* buffer);
bool deserialize_packet(const uint8_t* buffer, size_t received_size, protocol::Packet& packet);

}
