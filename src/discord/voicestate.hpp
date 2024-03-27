#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include "misc/bitwise.hpp"

// this is packed into a enum cuz it makes implementing tree models easier
enum class VoiceStateFlags : uint8_t {
    Clear = 0,
    Deaf = 1 << 0,
    Mute = 1 << 1,
    SelfDeaf = 1 << 2,
    SelfMute = 1 << 3,
    SelfStream = 1 << 4,
    SelfVideo = 1 << 5,
    Suppressed = 1 << 6,
};

struct PackedVoiceState {
    VoiceStateFlags Flags;
    std::optional<std::string> RequestToSpeakTimestamp;

    [[nodiscard]] bool IsSpeaker() const noexcept;
};

template<>
struct Bitwise<VoiceStateFlags> {
    static const bool enable = true;
};
