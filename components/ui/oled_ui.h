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
                    const char* second_phrase,
                    const char* third_phrase);
    void show_message(const char* message);
private:
    void slice_message(const char* message, uint8_t start, uint8_t end, char* output);
    display::OledDisplay& oled;
};

}
