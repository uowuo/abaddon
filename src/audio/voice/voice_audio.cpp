#include "voice_audio.hpp"

namespace AbaddonClient::Audio {

using VoicePlayback = Voice::VoicePlayback;
using VoiceCapture = Voice::VoiceCapture;

VoiceAudio::VoiceAudio(Context &context, DiscordClient &discord) noexcept :
    m_playback(context, discord),
    m_capture(context)
{
    auto& voice_client = discord.GetVoiceClient();

    voice_client.signal_connected()
        .connect(sigc::mem_fun(*this, &VoiceAudio::Start));

    voice_client.signal_disconnected()
        .connect(sigc::mem_fun(*this, &VoiceAudio::Stop));
}

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
