#include "mesh_log.h"

#include <cstdio>

#include "esp_log.h"
#include "packet_text.h"

constexpr char TAG[] = "mesh";

namespace mesh {

namespace {

const char* const kVerdict[] = {
    "new · own group",
    "duplicate or stale · own group",
    "new · other group",
    "duplicate or stale · other group",
    "own transmission heard back",
    "unknown sender, tracking table full",
};
static_assert(sizeof(kVerdict) / sizeof(kVerdict[0]) ==
                  static_cast<size_t>(ScreenerClassification::RejectedNoCapacity) + 1,
              "kVerdict must have one entry per ScreenerClassification");

void log_transmitted_packet(const protocol::Packet& packet, const char* kind, const char* footer) {
    ESP_LOGI(TAG,
             "\n"
             "┌─ TX · %-11s ─────────────────────\n"
             "│ from     %s · seq %" PRIu32 "\n"
             "│ group    %s · hops %u/%u left\n"
             "│ payload  %s\n"
             "└─ %s",
             kind,
             protocol::format::device_id(packet.header.message_id.origin_device_id).chars,
             packet.header.message_id.sequence_num,
             protocol::format::group_id(packet.header.message_id.group_id).chars,
             static_cast<unsigned>(packet.header.hop_limit),
             static_cast<unsigned>(protocol::kHopLimitDefault),
             protocol::format::payload(packet.payload).chars,
             footer);
}

}

void log_relay_failed(const protocol::Packet& packet) {
    ESP_LOGW(TAG, "relay failed for %s seq %" PRIu32,
             protocol::format::device_id(packet.header.message_id.origin_device_id).chars,
             packet.header.message_id.sequence_num);
}

void log_frame_size_mismatch(const radio::RadioResult& frame) {
    ESP_LOGW(TAG, "frame ignored: %u bytes on air, expected %u (%.0f dBm)",
             static_cast<unsigned>(frame.received_size),
             static_cast<unsigned>(protocol::kSerializedPacketSize),
             frame.rssi_dbm);
}

void log_node_identity(const LocalNodeState& state, const bool restored_from_nvs) {
    ESP_LOGI(TAG,
             "\n"
             "┌─ node ─────────────────────────────────\n"
             "│ device   %s\n"
             "│ group    %s\n"
             "│ next seq %" PRIu32 " (%s)\n"
             "│ protocol v%u · dictionary v%u · hop limit %u\n"
             "└─ identity ready",
             protocol::format::device_id(state.device_id).chars,
             protocol::format::group_id(state.group_id).chars,
             state.next_sequence_num,
             restored_from_nvs ? "restored from NVS" : "fresh node",
             static_cast<unsigned>(protocol::kCurrentProtocolVersion),
             static_cast<unsigned>(protocol::kCurrentPhraseDictionaryVersion),
             static_cast<unsigned>(protocol::kHopLimitDefault));
}

void log_received_packet(const IncomingPacketResult& incoming, const radio::RadioResult& frame) {
    const protocol::Packet& packet = incoming.packet;
    const char* verdict = kVerdict[static_cast<size_t>(incoming.classification)];
    const char* action = incoming.should_deliver
                             ? (incoming.should_relay ? "deliver + relay" : "deliver")
                             : (incoming.should_relay ? "relay" : "dropped");

    if (!incoming.should_deliver && !incoming.should_relay) {
        ESP_LOGI(TAG, "RX · %s seq %" PRIu32 " · %.0f dBm · %s → %s",
                 protocol::format::device_id(packet.header.message_id.origin_device_id).chars,
                 packet.header.message_id.sequence_num,
                 frame.rssi_dbm,
                 verdict,
                 action);
        return;
    }

    ESP_LOGI(TAG,
             "\n"
             "┌─ RX ───────────────────────────────────\n"
             "│ from     %s · seq %" PRIu32 "\n"
             "│ group    %s · hops %u/%u left\n"
             "│ signal   %.0f dBm · SNR %.1f dB · %u bytes\n"
             "│ version  protocol v%u · dictionary v%u\n"
             "│ payload  %s\n"
             "└─ %s → %s",
             protocol::format::device_id(packet.header.message_id.origin_device_id).chars,
             packet.header.message_id.sequence_num,
             protocol::format::group_id(packet.header.message_id.group_id).chars,
             static_cast<unsigned>(packet.header.hop_limit),
             static_cast<unsigned>(protocol::kHopLimitDefault),
             frame.rssi_dbm,
             frame.snr_db,
             static_cast<unsigned>(frame.received_size),
             static_cast<unsigned>(packet.header.protocol_version),
             static_cast<unsigned>(packet.header.phrase_dictionary_version),
             protocol::format::payload(packet.payload).chars,
             verdict,
             action);
}

void log_own_message_sent(const protocol::Packet& packet) {
    log_transmitted_packet(packet, "own message", "on air");
}

void log_relayed_packet_sent(const protocol::Packet& packet, const uint32_t backoff_ms) {
    char footer[48];
    snprintf(footer, sizeof(footer), "forwarded after %" PRIu32 " ms backoff", backoff_ms);

    log_transmitted_packet(packet, "relay", footer);
}

}
