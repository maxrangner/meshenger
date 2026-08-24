#pragma once

#include "device_state.h"
#include "packet_screener.h"
#include "packet.h"

namespace mesh {

enum class IncomingPacketAction {
    DeliverAndRelay,
    Deliver,
    Relay,
    Discard
};

struct IncomingPacketResult{
    IncomingPacketAction action;
    protocol::Packet packet;
};

struct OutgoingPacketResult {
    protocol::Packet packet;
    DeviceState updated_state;
};
    
class MeshCore {
    DeviceState device_state;
    PacketScreener screener;
public:
    void set_device_state(const DeviceState loaded_state);
    DeviceState get_device_state();
    OutgoingPacketResult create_outgoing_packet(const protocol::Payload& payload);
    bool prepare_packet_for_relay(protocol::Packet& packet);
    IncomingPacketResult process_incoming_packet(const protocol::Packet incoming_packet);
};

}
