#pragma once

#include "packet.h"

namespace protocol {

constexpr const uint8_t kPacketScreenerBufferSize = 100;

enum class ScreenerResult {
    SCR_OK,
    SCR_ERROR_DUPLICATE,
    SCR_ERROR_WRONG_GROUP
};

class PacketScreener {
    uint64_t current_group_id;
    protocol::MessageId newest_packets_seen[kPacketScreenerBufferSize] = {};
    uint8_t seen_unit_count = 0;
public:
    PacketScreener(uint64_t group_id);
    ScreenerResult screen_packet(const protocol::Packet& packet);
};

bool is_packet_for_group(uint64_t group_id, const Packet& packet);

}