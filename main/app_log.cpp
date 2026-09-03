#include "app_log.h"

#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "packet_text.h"
#include "phrase_dictionary.h"

constexpr char BOOT_TAG[] = "main";
constexpr char TAG[] = "app";

namespace app {

namespace {

const char* reset_reason_text(const esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON:    return "power on";
        case ESP_RST_SW:         return "software restart";
        case ESP_RST_PANIC:      return "panic";
        case ESP_RST_INT_WDT:    return "interrupt watchdog";
        case ESP_RST_TASK_WDT:   return "task watchdog";
        case ESP_RST_WDT:        return "other watchdog";
        case ESP_RST_BROWNOUT:   return "brownout";
        case ESP_RST_DEEPSLEEP:  return "deep sleep wake";
        default:                 return "unknown";
    }
}

}

void log_boot_banner() {
    const esp_app_desc_t* app_description = esp_app_get_description();

    ESP_LOGI(BOOT_TAG,
             "\n"
             "┌─ Meshenger ────────────────────────────\n"
             "│ firmware %s · built %s\n"
             "│ idf      %s\n"
             "│ reset    %s\n"
             "│ heap     %u kB free\n"
             "└─ booting",
             app_description->version,
             app_description->date,
             app_description->idf_ver,
             reset_reason_text(esp_reset_reason()),
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_DEFAULT) / 1024));
}

void log_status_update_sending(const uint8_t first_phrase, const uint8_t second_phrase) {
    ESP_LOGI(TAG, "sending status update: \"%s %s\"",
             kPhraseDictionaryV1[first_phrase],
             kPhraseDictionaryV1[second_phrase]);
}

void log_status_update_received(const uint64_t origin_device_id,
                                const uint8_t first_phrase,
                                const uint8_t second_phrase) {
    ESP_LOGI(TAG, "status update from %s: \"%s %s\"",
             protocol::format::device_id(origin_device_id).chars,
             kPhraseDictionaryV1[first_phrase],
             kPhraseDictionaryV1[second_phrase]);
}

void log_status_update_out_of_range(const uint64_t origin_device_id,
                                    const uint8_t first_phrase,
                                    const uint8_t second_phrase) {
    ESP_LOGW(TAG, "status update from %s discarded: phrase id %u/%u outside dictionary v%u (%u phrases)",
             protocol::format::device_id(origin_device_id).chars,
             static_cast<unsigned>(first_phrase),
             static_cast<unsigned>(second_phrase),
             static_cast<unsigned>(kPhraseDictionaryVersion),
             static_cast<unsigned>(kPhraseCountV1));
}

}
