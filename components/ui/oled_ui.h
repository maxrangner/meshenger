#pragma once

#include "oled_display.h"
#include "packet.h"

namespace display {

enum class DebugDirection {
    Sent,
    Received,
};

class OledUi {
public:
    OledUi(OledDisplay& display);
    void show_debug(DebugDirection direction,
                    uint64_t origin_device_id,
                    protocol::Payload payload,
                    const char* first_phrase,
                    const char* second_phrase);
private:
    display::OledDisplay& oled;
};

}
