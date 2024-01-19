#pragma once
#include <cstdint>
#include "misc/bitwise.hpp"

enum class VoiceStateFlags : uint8_t {
    Clear = 0,
    Deaf = 1 << 0,
    Mute = 1 << 1,
    SelfDeaf = 1 << 2,
    SelfMute = 1 << 3,
    SelfStream = 1 << 4,
    SelfVideo = 1 << 5,
};

template<>
struct Bitwise<VoiceStateFlags> {
    static const bool enable = true;
};
