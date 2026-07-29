#pragma once

#include "packet.h"

enum class AppEventMessage {
    BUTTON_SHORT_PRESS,
    BUTTON_LONG_PRESS,
    MESSAGE_RECEIVED
};

struct AppEvent {
    AppEventMessage message;
    uint8_t payload[protocol::kPacketSize];
};