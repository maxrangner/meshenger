#include "mesh_core.h"

#include <cstring>
#include "esp_log.h"

namespace mesh {

constexpr char TAG[] = "mesh_core";

void MeshCore::set_device_state(DeviceState loaded_state) {
    device_state = loaded_state;
    screener.set_device_state(loaded_state.group_id, loaded_state.device_id);
}

OutgoingResult MeshCore::create_outgoing_packet(const protocol::Payload& payload) {
    protocol::Packet packet{};

    packet.version = device_state.protocol_version;
    packet.message_id.group_id = device_state.group_id;
    packet.message_id.origin_device_id = device_state.device_id;
    packet.message_id.message_num = device_state.packet_count++;
    packet.payload = payload;

    return {packet, device_state};
}

IncomingResult MeshCore::process_incoming_packet(protocol::Packet incoming_packet) {
    ScreenerResult result = screener.process_packet(incoming_packet);
    switch (result) {
        case ScreenerResult::SAME_GROUP_NEW:
            ESP_LOGI(TAG, "ScreenerResult::SAME_GROUP_NEW");
            return {IncomingResultMessage::DELIVER_AND_RELAY, incoming_packet};
        // case ScreenerResult::SAME_GROUP_OLD:
        //     return {IncomingResultMessage::DISCARD, protocol::Packet{}};
        case ScreenerResult::OTHER_GROUP_NEW:
            ESP_LOGI(TAG, "ScreenerResult::OTHER_GROUP_NEW");
            return {IncomingResultMessage::RELAY, incoming_packet};
        // case ScreenerResult::OTHER_GROUP_OLD:
        //     return {IncomingResultMessage::DISCARD, protocol::Packet{}};
        // case ScreenerResult::DISCARD:
        //     return {IncomingResultMessage::DISCARD, protocol::Packet{}};
        default:
            ESP_LOGI(TAG, "ScreenerResult::DISCARD");
            return {IncomingResultMessage::DISCARD, protocol::Packet{}};
    }
}

}