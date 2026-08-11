#pragma once

#include "packet.h"

namespace mesh {

constexpr uint8_t kPacketScreenerBufferSize = 30;

enum class ScreenerResult {
    SAME_GROUP_NEW,
    SAME_GROUP_RELAY,
    OTHER_GROUP_NEW,
    OTHER_GROUP_RELAY,
    DISCARD
};

class PacketScreener {
    uint64_t current_group_id;
    protocol::MessageId newest_packets_seen[kPacketScreenerBufferSize] = {};
    uint8_t seen_unit_count = 0;
public:
    PacketScreener();
    ScreenerResult screen_packet(const protocol::Packet& packet);
};

bool is_packet_for_group(uint64_t group_id, const protocol::Packet& packet);

}