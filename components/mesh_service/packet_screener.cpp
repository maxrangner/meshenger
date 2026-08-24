#include "packet_screener.h"

namespace mesh {

void PacketScreener::set_device_state(const uint64_t group, const uint64_t device) {
    local_group_id = group;
    local_device_id = device;
}

ScreenerResult PacketScreener::screen_packet(const protocol::Packet& packet) {
    if (is_current_device(packet)) return ScreenerResult::Rejected;

    for (int i = 0; i < tracked_source_counts; i++) {
        if (is_packet_matching(packet, i)) {
            if (is_packet_fresh(packet, i)) {
                latest_message_ids[i] = packet.header.message_id;
                if (is_packet_for_group(packet)) {
                    return ScreenerResult::NewLocalGroupPacket;
                } else {
                    return ScreenerResult::NewForeignGroupPacket;
                }
            } else {
                if (is_packet_for_group(packet)) {
                    return ScreenerResult::StaleLocalGroupPacket;
                } else {
                    return ScreenerResult::StaleForeignGroupPacket;
                }
            }
        }
    }

    if (is_buffer_full()) {
        latest_message_ids[tracked_source_counts++] = packet.header.message_id;
        if (is_packet_for_group(packet)) {
            return ScreenerResult::NewLocalGroupPacket;
        } else {
            return ScreenerResult::NewForeignGroupPacket;
        }
    } else {
        return ScreenerResult::Rejected;
    }
}

bool PacketScreener::is_current_device(const protocol::Packet& packet) const {
    return (packet.header.message_id.origin_device_id == local_device_id);
}

bool PacketScreener::is_packet_for_group(const protocol::Packet& packet) const {
    return (packet.header.message_id.group_id == local_group_id);
}

bool PacketScreener::is_packet_matching(const protocol::Packet& packet, uint8_t i) const {
    return (packet.header.message_id.group_id == latest_message_ids[i].group_id && packet.header.message_id.origin_device_id == latest_message_ids[i].origin_device_id);
}

bool PacketScreener::is_packet_fresh(const protocol::Packet& packet, uint8_t i) const {
    return (packet.header.message_id.sequence_num > latest_message_ids[i].sequence_num);
}

bool PacketScreener::is_buffer_full() {
    return (tracked_source_counts < kMaxTrackedSources);
}

}