#pragma once

#include "device_state.h"
#include "packet_screener.h"
#include "packet.h"

namespace mesh {

enum class IncomingResultMessage {
    DELIVER,
    RELAY,
    DISCARD
};

struct IncomingResult{
    IncomingResultMessage message;
    protocol::Packet packet;
};

struct OutgoingResult {
    protocol::Packet packet;
    DeviceState state;
};
    
class MeshCore {
    DeviceState device_state;
    PacketScreener screener;
public:
    void set_device_state(DeviceState loaded_state);
    OutgoingResult create_packet(const uint8_t *payload);
    IncomingResult process_incoming_packet(protocol::Packet incoming_packet);
};

}
