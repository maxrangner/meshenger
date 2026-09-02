#include "packet_screener.h"

namespace mesh {

void PacketScreener::set_local_identity(const uint64_t group, const uint64_t device) {
    local_group_id = group;
    local_device_id = device;
}

ScreenerClassification PacketScreener::screen_packet(const protocol::Packet& packet) {
    if (is_from_local_device(packet)) return ScreenerClassification::Rejected;

    for (int source_index = 0; source_index < tracked_source_count; source_index++) {
        if (is_packet_matching(packet, source_index)) {
            if (is_packet_fresh(packet, source_index)) {
                latest_message_ids[source_index] = packet.header.message_id;
                if (is_packet_for_local_group(packet)) {
                    return ScreenerClassification::NewLocalGroupPacket;
                } else {
                    return ScreenerClassification::NewForeignGroupPacket;
                }
            } else {
                if (is_packet_for_local_group(packet)) {
                    return ScreenerClassification::StaleLocalGroupPacket;
                } else {
                    return ScreenerClassification::StaleForeignGroupPacket;
                }
            }
        }
    }

    if (has_buffer_capacity()) {
        latest_message_ids[tracked_source_count++] = packet.header.message_id;
        if (is_packet_for_local_group(packet)) {
            return ScreenerClassification::NewLocalGroupPacket;
        } else {
            return ScreenerClassification::NewForeignGroupPacket;
        }
    } else {
        return ScreenerClassification::Rejected;
    }
}

bool PacketScreener::is_from_local_device(const protocol::Packet& packet) const {
    return (packet.header.message_id.origin_device_id == local_device_id);
}

bool PacketScreener::is_packet_for_local_group(const protocol::Packet& packet) const {
    return (packet.header.message_id.group_id == local_group_id);
}

bool PacketScreener::is_packet_matching(const protocol::Packet& packet, uint8_t source_index) const {
    return (packet.header.message_id.group_id == latest_message_ids[source_index].group_id && packet.header.message_id.origin_device_id == latest_message_ids[source_index].origin_device_id);
}

bool PacketScreener::is_packet_fresh(const protocol::Packet& packet, uint8_t source_index) const {
    return (packet.header.message_id.sequence_num > latest_message_ids[source_index].sequence_num);
}

bool PacketScreener::has_buffer_capacity() {
    return (tracked_source_count < kMaxTrackedSources);
}

}