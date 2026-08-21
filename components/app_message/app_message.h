#pragma once

#include "packet.h"

namespace app {

enum class AppEventMessage {
    BUTTON_SHORT_PRESS,
    BUTTON_LONG_PRESS,
    MESSAGE_RECEIVED
};

struct AppEvent {
    AppEventMessage message;
    uint64_t origin_device_id;
    protocol::Payload payload;
};

}
