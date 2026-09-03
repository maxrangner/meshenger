#pragma once

#include "node_state.h"
#include "packet_screener.h"
#include "packet.h"

namespace mesh {

struct IncomingPacketResult{
    bool should_deliver = false;
    bool should_relay = false;
    ScreenerClassification classification = ScreenerClassification::RejectedOwnPacket;
    protocol::Packet packet;
};

struct OutgoingPacketResult {
    protocol::Packet packet;
    LocalNodeState updated_state;
};
    
class MeshCore {
public:
    void set_local_identity(const LocalNodeState loaded_state);
    LocalNodeState get_node_state();
    OutgoingPacketResult create_outgoing_packet(const protocol::Payload& payload);
    bool prepare_packet_for_relay(protocol::Packet& packet);
    IncomingPacketResult process_incoming_packet(const protocol::Packet incoming_packet);
private:
    bool packet_is_compatible(const protocol::Packet& packet);
    LocalNodeState node_state;
    PacketScreener screener;
};

}
