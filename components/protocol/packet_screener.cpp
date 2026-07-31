#include "packet_screener.h"

namespace protocol {

bool is_packet_for_group(uint64_t group_id, const Packet& packet) {
    return (group_id == packet.message_id.group_id);
}

}