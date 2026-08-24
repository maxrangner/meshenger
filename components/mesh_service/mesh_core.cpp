#include "mesh_core.h"

#include <cstring>
#include "esp_log.h"

namespace mesh {

constexpr char TAG[] = "mesh_core";

void MeshCore::set_device_state(const DeviceState loaded_state) {
    device_state = loaded_state;
    screener.set_device_state(loaded_state.group_id, loaded_state.device_id);
}

DeviceState MeshCore::get_device_state() {
    return device_state;
}

OutgoingPacketResult MeshCore::create_outgoing_packet(const protocol::Payload& payload) {
    protocol::Packet packet{};

    packet.header.protocol_version = device_state.protocol_version;
    packet.header.phrase_dictionary_version = device_state.phrase_dictionary_version;
    packet.header.message_id.group_id = device_state.group_id;
    packet.header.message_id.origin_device_id = device_state.device_id;
    packet.header.message_id.sequence_num = device_state.next_sequence_num++;
    packet.payload = payload;

    return {packet, device_state};
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
    ScreenerResult result = screener.screen_packet(incoming_packet);

    switch (result) {
        case ScreenerResult::NewLocalGroupPacket:
            ESP_LOGI(TAG, "ScreenerResult::NewLocalGroupPacket");
            if (incoming_packet.header.hop_limit == 0) {
                return {IncomingPacketAction::Deliver, incoming_packet};
            } else {
                return {IncomingPacketAction::DeliverAndRelay, incoming_packet};
            }
            break;
        // case ScreenerResult::StaleLocalGroupPacket:
        //     return {IncomingPacketAction::Discard, protocol::Packet{}};
        case ScreenerResult::NewForeignGroupPacket:
            ESP_LOGI(TAG, "ScreenerResult::NewForeignGroupPacket");
            if (incoming_packet.header.hop_limit != 0) {
                return {IncomingPacketAction::Relay, incoming_packet};
            } else {
                ESP_LOGI(TAG, "ScreenerResult::Rejected");
                return {IncomingPacketAction::Discard, protocol::Packet{}};
            }
            break;
        // case ScreenerResult::StaleForeignGroupPacket:
        //     return {IncomingPacketAction::Discard, protocol::Packet{}};
        // case ScreenerResult::Rejected:
        //     return {IncomingPacketAction::Discard, protocol::Packet{}};
        default:
            ESP_LOGI(TAG, "ScreenerResult::Rejected");
            return {IncomingPacketAction::Discard, protocol::Packet{}};
    }
}

}
