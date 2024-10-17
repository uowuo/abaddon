#pragma once

#include <sigc++/sigc++.h>

#include "discord/snowflake.hpp"
#include "discord/discord.hpp"
#include "discord/voiceclient.hpp"

#include "audio/audio_device.hpp"
#include "audio/context.hpp"
#include "audio/utils.hpp"

#include "client_store.hpp"

namespace AbaddonClient::Audio::Voice {

class VoicePlayback : public sigc::trackable  {
public:
    using ClientID = Playback::ClientStore::ClientID;

    VoicePlayback(Context &context, DiscordClient &discord) noexcept;

    void OnOpusData(ClientID id, std::vector<uint8_t> &&data) noexcept;

    void Start() noexcept;
    void Stop() noexcept;

    void SetActive(bool active) noexcept;
    bool GetActive() const noexcept;

    void SetPlaybackDevice(const ma_device_id &device_id) noexcept;

    Playback::ClientStore& GetClientStore() noexcept;
    const Playback::ClientStore& GetClientStore() const noexcept;

private:
    void OnAudioPlayback(OutputBuffer buffer) noexcept;

    void OnUserSpeaking(const VoiceSpeakingData &speaking_data) noexcept;
    void OnUserDisconnect(Snowflake user_id, Snowflake channel_id) noexcept;

    ma_device_config GetDeviceConfig() noexcept;

    DiscordVoiceClient &m_voice_client;

    AudioDevice m_device;
    Playback::ClientStore m_clients;

    std::atomic<bool> m_active = true;

    friend void playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};

};
