#include "voice_audio.hpp"

namespace AbaddonClient::Audio {

using VoicePlayback = Voice::VoicePlayback;
using VoiceCapture = Voice::VoiceCapture;

VoiceAudio::VoiceAudio(Context &context) noexcept :
    m_playback(context),
    m_capture(context) {}

void VoiceAudio::Start() noexcept {
    m_playback.Start();
    m_capture.Start();
}

void VoiceAudio::Stop() noexcept {
    m_playback.Stop();
    m_capture.Stop();
}

VoicePlayback& VoiceAudio::GetPlayback() noexcept {
    return m_playback;
}

const VoicePlayback& VoiceAudio::GetPlayback() const noexcept {
    return m_playback;
}

VoiceCapture& VoiceAudio::GetCapture() noexcept {
    return m_capture;
}

const VoiceCapture& VoiceAudio::GetCapture() const noexcept {
    return m_capture;
}

}
