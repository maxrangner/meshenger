#pragma once

#include "packet.h"

namespace mesh {

constexpr uint8_t kPacketScreenerBufferSize = 30;

enum class ScreenerResult {
    SAME_GROUP_NEW,
    SAME_GROUP_OLD,
    OTHER_GROUP_NEW,
    OTHER_GROUP_OLD,
    DISCARD
};

class PacketScreener {
    uint64_t current_group_id;
    uint64_t current_device_id;
    protocol::MessageId newest_packets_seen[kPacketScreenerBufferSize]{};
    uint8_t seen_unit_count = 0;
public:
    void set_device_state(const uint64_t group, const uint64_t device);
    ScreenerResult process_packet(const protocol::Packet& packet);
};

bool is_packet_for_group(uint64_t group_id, const protocol::Packet& packet);

}