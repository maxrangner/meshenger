#include "packet_screener.h"

namespace protocol {

PacketScreener::PacketScreener(uint64_t group_id): current_group_id(group_id) {}

ScreenerResult PacketScreener::screen_packet(const protocol::Packet& packet) {
    ScreenerResult result;

    for (int i = 0; i < seen_unit_count; i++) {
        if (packet.message_id.group_id == newest_packets_seen[i].group_id && packet.message_id.origin_device_id == newest_packets_seen[i].origin_device_id) {
            if (packet.message_id.message_num > newest_packets_seen[i].message_num) {
                newest_packets_seen[i] = packet.message_id;
                return ScreenerResult::SCR_OK;
            } else {
                return ScreenerResult::SCR_ERROR_DUPLICATE;
            }
        }
    }

    if (seen_unit_count < kPacketScreenerBufferSize) {
        newest_packets_seen[seen_unit_count++] = packet.message_id;
    }

    return ScreenerResult::SCR_OK;;
}

bool is_packet_for_group(uint64_t group_id, const Packet& packet) {
    return (group_id == packet.message_id.group_id);
}

}