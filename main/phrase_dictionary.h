#pragma once

#include <cstdint>

namespace app {

const uint8_t kPhraseCountV1 = 3;
const uint8_t kPhraseDictionaryVersion = 1;

static const char *const kPhraseDictionaryV1[kPhraseCountV1] = {
    "I am safe.",
    "Finding shelter.",
    "Have food."
};

}
