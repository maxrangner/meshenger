#include "packet_screener.h"

namespace mesh {

void PacketScreener::set_device_state(const uint64_t group, const uint64_t device) {
    current_group_id = group;
    current_device_id = device;
}

ScreenerResult PacketScreener::process_packet(const protocol::Packet& packet) {
    if (packet.message_id.origin_device_id == current_device_id) return ScreenerResult::DISCARD;
    
    for (int i = 0; i < seen_unit_count; i++) {
        if (packet.message_id.group_id == newest_packets_seen[i].group_id && packet.message_id.origin_device_id == newest_packets_seen[i].origin_device_id) {
            if (packet.message_id.message_num > newest_packets_seen[i].message_num) {
                newest_packets_seen[i] = packet.message_id;
                if (packet.message_id.group_id == current_group_id) {
                    return ScreenerResult::SAME_GROUP_NEW;
                } else {
                    return ScreenerResult::OTHER_GROUP_NEW;
                }
            } else {
                if (packet.message_id.group_id == current_group_id) {
                    return ScreenerResult::SAME_GROUP_OLD;
                } else {
                    return ScreenerResult::OTHER_GROUP_OLD;
                }
            }
        }
    }

    if (seen_unit_count < kPacketScreenerBufferSize) {
        newest_packets_seen[seen_unit_count++] = packet.message_id;
        if (packet.message_id.group_id == current_group_id) {
            return ScreenerResult::SAME_GROUP_NEW;
        } else {
            return ScreenerResult::OTHER_GROUP_NEW;
        }
    } else {
        return ScreenerResult::DISCARD;
    }
}

bool is_packet_for_group(uint64_t group_id, const protocol::Packet& packet) {
    return (group_id == packet.message_id.group_id);
}

}