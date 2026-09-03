#pragma once

#include "mesh_core.h"
#include "node_state.h"
#include "packet.h"
#include "radio_service.h"

namespace mesh {

void log_node_identity(const LocalNodeState& state, const bool restored_from_nvs);

void log_own_message_sent(const protocol::Packet& packet);

void log_frame_size_mismatch(const radio::RadioResult& frame);

void log_received_packet(const IncomingPacketResult& incoming, const radio::RadioResult& frame);

void log_relay_failed(const protocol::Packet& packet);

void log_relayed_packet_sent(const protocol::Packet& packet, const uint32_t backoff_ms);

}
