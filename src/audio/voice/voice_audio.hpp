#pragma once

#include "audio/context.hpp"

#include "capture/voice_capture.hpp"
#include "playback/voice_playback.hpp"

namespace AbaddonClient::Audio {

class VoiceAudio {
public:
    using VoicePlayback = Voice::VoicePlayback;
    using VoiceCapture = Voice::VoiceCapture;

    VoiceAudio(Context &context) noexcept;

    void Start() noexcept;
    void Stop() noexcept;

    VoicePlayback& GetPlayback() noexcept;
    const VoicePlayback& GetPlayback() const noexcept;

    VoiceCapture& GetCapture() noexcept;
    const VoiceCapture& GetCapture() const noexcept;

private:
    VoiceCapture m_capture;
    VoicePlayback m_playback;
};

}
