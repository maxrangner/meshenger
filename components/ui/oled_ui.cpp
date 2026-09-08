#include "oled_ui.h"

#include "packet_text.h"

namespace display {

OledUi::OledUi(OledDisplay& display) : oled(display) {}

void OledUi::show_debug(const DebugDirection direction,
                        const uint64_t origin_device_id,
                        const protocol::Payload payload,
                        const char* first_phrase,
                        const char* second_phrase) {
    oled.clear();

    uint8_t content_y = 8;
    if (direction == DebugDirection::Sent) {
        oled.display_text(0, 0, "PACKET SENT");
    } else {
        oled.display_text(0, 0, "PACKET RECEIVED");
        oled.display_text(0, 8, "ID:");
        oled.display_text(24, 8, protocol::format::device_id(origin_device_id).chars);
        content_y = 16;
    }

    oled.display_text(0, content_y, "DATA:");
    oled.display_text(36, content_y, protocol::format::payload(payload).chars);
    oled.display_text(0, content_y + 8, first_phrase);
    oled.display_text(0, content_y + 16, second_phrase);
}

}
