#include "voice_playback.hpp"
#include "constants.hpp"

namespace AbaddonClient::Audio::Voice {

void playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    auto playback = static_cast<VoicePlayback*>(pDevice->pUserData);
    if (playback == nullptr) {
        return;
    }

    auto buffer = OutputBuffer(static_cast<float*>(pOutput), frameCount * RTP_CHANNELS);
    playback->OnAudioPlayback(buffer);
}

VoicePlayback::VoicePlayback(Context &context, DiscordClient &discord) noexcept :
    m_device(context, GetDeviceConfig(), context.GetActivePlaybackID()),
    m_voice_client(discord.GetVoiceClient())
{
    m_voice_client.signal_speaking()
        .connect(sigc::mem_fun(*this, &VoicePlayback::OnUserSpeaking));

    discord.signal_voice_user_disconnect()
        .connect(sigc::mem_fun(*this, &VoicePlayback::OnUserDisconnect));
}

void VoicePlayback::OnRTPData(ClientID id, std::vector<uint8_t> &&data) noexcept {
    if (m_active) {
        m_clients.DecodeFromRTP(id, std::move(data));
    }
}

void VoicePlayback::OnAudioPlayback(OutputBuffer buffer) noexcept {
    if (m_active) {
        m_clients.WriteMixed(buffer);
        AudioUtils::ClampToFloatRange(buffer); // Clamp it at the end
    }
}

void VoicePlayback::OnUserSpeaking(const VoiceSpeakingData &speaking_data) noexcept {
    m_clients.AddClient(speaking_data.SSRC);

    const auto volume = m_voice_client.GetUserVolume(speaking_data.UserID);
    m_clients.SetClientVolume(speaking_data.SSRC, volume);
}

void VoicePlayback::OnUserDisconnect(Snowflake user_id, Snowflake channel_id) noexcept {
    const auto ssrc = m_voice_client.GetSSRCOfUser(user_id);
    if (ssrc) {
        m_clients.RemoveClient(*ssrc);
    }
}

void VoicePlayback::Start() noexcept {
    if (Abaddon::Get().GetSettings().SeparateSources) {
        return;
    }

    m_device.Start();
}

void VoicePlayback::Stop() noexcept {
    m_device.Stop();
}

void VoicePlayback::SetActive(bool active) noexcept {
    if (!active) {
        // Clear all buffers to prevent residue samples playing back later
        m_clients.ClearAllBuffers();
    }

    m_active = active;
}

bool VoicePlayback::GetActive() const noexcept {
    return m_active;
}

void VoicePlayback::SetPlaybackDevice(const ma_device_id &device_id) noexcept {
    spdlog::get("voice")->info("Setting playback device");

    const auto success = m_device.ChangeDevice(device_id);
    if (!success) {
        spdlog::get("voice")->error("Failed to set playback device");
    }
}

Playback::ClientStore& VoicePlayback::GetClientStore() noexcept {
    return m_clients;
}

const Playback::ClientStore& VoicePlayback::GetClientStore() const noexcept {
    return m_clients;
}

ma_device_config VoicePlayback::GetDeviceConfig() noexcept {
    auto config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = RTP_FORMAT;
    config.sampleRate = RTP_SAMPLE_RATE;
    config.playback.channels = RTP_CHANNELS;
    config.pulse.pStreamNamePlayback = "Abaddon (Voice)";

    config.noClip = true;
    config.noFixedSizedCallback = true;
    config.performanceProfile = ma_performance_profile_low_latency;

    config.dataCallback = playback_callback;
    config.pUserData = this;

    return config;
}

}
