#pragma once

#include "packet.h"

namespace utils {

void serializePacket(const protocol::Packet& packet, uint8_t* buffer);
void deserializePacket(const uint8_t* buffer, protocol::Packet& packet);

}