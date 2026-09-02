#include "mesh_core.h"

#include <cstring>
#include "esp_log.h"

namespace mesh {

constexpr char TAG[] = "mesh_core";

void MeshCore::set_local_identity(const LocalNodeState loaded_state) {
    node_state = loaded_state;
    screener.set_local_identity(loaded_state.group_id, loaded_state.device_id);
}

LocalNodeState MeshCore::get_node_state() {
    return node_state;
}

OutgoingPacketResult MeshCore::create_outgoing_packet(const protocol::Payload& payload) {
    protocol::Packet packet{};

    packet.header.protocol_version = node_state.protocol_version;
    packet.header.phrase_dictionary_version = node_state.phrase_dictionary_version;
    packet.header.message_id.group_id = node_state.group_id;
    packet.header.message_id.origin_device_id = node_state.device_id;
    packet.header.message_id.sequence_num = node_state.next_sequence_num++;
    packet.payload = payload;

    return {packet, node_state};
}

bool MeshCore::prepare_packet_for_relay(protocol::Packet& packet) {
    if (packet.header.hop_limit != 0) {
        packet.header.hop_limit--;
        return true;
    } else {
        return false;
    }
}

IncomingPacketResult MeshCore::process_incoming_packet(const protocol::Packet incoming_packet) {
    ScreenerClassification result = screener.screen_packet(incoming_packet);

    switch (result) {
        case ScreenerClassification::NewLocalGroupPacket:
            ESP_LOGI(TAG, "ScreenerClassification::NewLocalGroupPacket");
            if (incoming_packet.header.hop_limit == 0) {
                return {IncomingPacketAction::Deliver, incoming_packet};
            } else {
                return {IncomingPacketAction::DeliverAndRelay, incoming_packet};
            }
            break;
        // case ScreenerClassification::StaleLocalGroupPacket:
        //     return {IncomingPacketAction::Discard, protocol::Packet{}};
        case ScreenerClassification::NewForeignGroupPacket:
            ESP_LOGI(TAG, "ScreenerClassification::NewForeignGroupPacket");
            if (incoming_packet.header.hop_limit != 0) {
                return {IncomingPacketAction::Relay, incoming_packet};
            } else {
                ESP_LOGI(TAG, "ScreenerClassification::Rejected");
                return {IncomingPacketAction::Discard, protocol::Packet{}};
            }
            break;
        // case ScreenerClassification::StaleForeignGroupPacket:
        //     return {IncomingPacketAction::Discard, protocol::Packet{}};
        // case ScreenerClassification::Rejected:
        //     return {IncomingPacketAction::Discard, protocol::Packet{}};
        default:
            ESP_LOGI(TAG, "ScreenerClassification::Rejected");
            return {IncomingPacketAction::Discard, protocol::Packet{}};
    }
}

}
