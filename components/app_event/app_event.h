#pragma once

#include "packet.h"

namespace app {

enum class AppEventType {
    ShortButtonPress,
    LongButtonPress,
    StatusUpdateReceived
};

struct AppEvent {
    AppEventType type;
    uint64_t origin_device_id;
    protocol::Payload payload;
};

}
