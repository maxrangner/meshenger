#include "oled_ui.h"

#include "packet_text.h"

namespace display {

OledUi::OledUi(OledDisplay& display) : oled(display) {}

void OledUi::show_debug(const DebugDirection direction,
                        const uint64_t origin_device_id,
                        const protocol::Payload payload,
                        const char* first_phrase,
                        const char* second_phrase,
                        const char* third_phrase) {
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
    oled.display_text(0, content_y + 24, third_phrase);
}

void OledUi::show_message(const char* message) {
    oled.clear();

    uint8_t line_start = 0;
    uint8_t last_space = 0;
    uint8_t current_y = 0;
    bool has_space = false;
    char buffer[17]{};

    // max rangnerrrrrr

    uint8_t i = 0;
    for (; message[i] != '\0' && current_y <= 56; i++) {
        if (message[i] == ' ') {
            last_space = i;
            has_space = true;
        }

        if ((i - line_start) >= 15) {
            if (has_space) {
                slice_message(message, line_start, last_space, buffer);
                oled.display_text(0, current_y, buffer);
                line_start = last_space + 1;
            } else {
                slice_message(message, line_start, line_start + 16, buffer);
                oled.display_text(0, current_y, buffer);
                line_start += 16;
            }

            has_space = false;
            i = line_start - 1;
            current_y += 8;
        }

    }

    if (message[i] == '\0' && i > line_start && current_y <= 56) {
        slice_message(message, line_start, i, buffer);
        oled.display_text(0, current_y, buffer);
    }
}

void OledUi::slice_message(const char* message, uint8_t start, uint8_t end, char* output) {
    uint8_t i = 0;
    for (; i < (end - start); i++) {
        output[i] = message[start + i];
    }
    output[i] = '\0';
}

}
