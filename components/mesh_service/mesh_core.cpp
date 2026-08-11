#include "mesh_core.h"

#include <cstring>

namespace mesh {

void MeshCore::set_device_state(DeviceState loaded_state) {
    device_state = loaded_state;
}

OutgoingResult MeshCore::create_packet(const uint8_t *payload) {
    protocol::Packet packet{};

    packet.version = device_state.protocol_version;
    packet.message_id.group_id = device_state.group_id;
    packet.message_id.origin_device_id = device_state.device_id;
    packet.message_id.message_num = device_state.packet_count++;

    memcpy(packet.payload, payload, protocol::kPayloadSize);

    return {packet, device_state};
}

IncomingResult MeshCore::process_incoming_packet(protocol::Packet incoming_packet) {
    screener.screen_packet(incoming_packet);
    IncomingResult result{};
    result.message = IncomingResultMessage::DELIVER;
    return result;
}

}