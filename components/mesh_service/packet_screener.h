#pragma once

#include "packet.h"

namespace mesh {

constexpr uint8_t kMaxTrackedSources = 30;

enum class ScreenerClassification {
    NewLocalGroupPacket,
    StaleLocalGroupPacket,
    NewForeignGroupPacket,
    StaleForeignGroupPacket,
    Rejected
};

class PacketScreener {
public:
    void set_local_identity(const uint64_t group_id, const uint64_t device_id);
    ScreenerClassification screen_packet(const protocol::Packet& packet);
private:
    bool is_from_local_device(const protocol::Packet& packet) const;
    bool is_packet_for_local_group(const protocol::Packet& packet) const;
    bool is_packet_matching(const protocol::Packet& packet, uint8_t i) const;
    bool is_packet_fresh(const protocol::Packet& packet, uint8_t i) const;
    bool has_buffer_capacity();

    uint64_t local_group_id = 0;
    uint64_t local_device_id = 0;
    protocol::MessageId latest_message_ids[kMaxTrackedSources]{};
    uint8_t tracked_source_count = 0;
};


}