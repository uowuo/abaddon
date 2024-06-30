#include "voicestate.hpp"

bool PackedVoiceState::IsSpeaker() const noexcept {
    return ((Flags & VoiceStateFlags::Suppressed) != VoiceStateFlags::Suppressed) && !RequestToSpeakTimestamp.has_value();
}
