#pragma once

#include "packet.h"

namespace mesh {

constexpr uint8_t kMaxTrackedSources = 30;

enum class ScreenerResult {
    NewLocalGroupPacket,
    StaleLocalGroupPacket,
    NewForeignGroupPacket,
    StaleForeignGroupPacket,
    Rejected
};

class PacketScreener {
    uint64_t local_group_id;
    uint64_t local_device_id;
    protocol::MessageId latest_message_ids[kMaxTrackedSources]{};
    uint8_t tracked_source_counts = 0;
    bool is_current_device(const protocol::Packet& packet) const;
    bool is_packet_for_group(const protocol::Packet& packet) const;
    bool is_packet_matching(const protocol::Packet& packet, uint8_t i) const;
    bool is_packet_fresh(const protocol::Packet& packet, uint8_t i) const;
    bool is_buffer_full();
public:
    void set_device_state(const uint64_t group, const uint64_t device);
    ScreenerResult screen_packet(const protocol::Packet& packet);
};


}