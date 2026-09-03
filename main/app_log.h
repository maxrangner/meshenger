#pragma once

#include <cstdint>

namespace app {

void log_boot_banner();

void log_status_update_sending(const uint8_t first_phrase, const uint8_t second_phrase);

void log_status_update_received(const uint64_t origin_device_id,
                                const uint8_t first_phrase,
                                const uint8_t second_phrase);

void log_status_update_out_of_range(const uint64_t origin_device_id,
                                    const uint8_t first_phrase,
                                    const uint8_t second_phrase);

}
