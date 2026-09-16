#pragma once

#include <cstdint>

namespace app {

const uint8_t kPhraseCountV1 = 11;
const uint8_t kPhraseDictionaryVersion = 1;

static const char *const kPhraseDictionaryV1[kPhraseCountV1] = {
    "I am safe.",
    "I am in danger.",

    "Finding water.",
    "Finding food.",
    "Finding shelter.",

    "Have water.",
    "Have food.",
    "Have shelter.",

    "Come to me.",
    "Don't come to me."
    "Find me."
};

}
