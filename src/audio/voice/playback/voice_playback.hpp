#pragma once

#include "audio/audio_device.hpp"
#include "audio/context.hpp"
#include "audio/utils.hpp"

#include "client_store.hpp"

namespace AbaddonClient::Audio::Voice {

class VoicePlayback {
public:
    using ClientID = Playback::ClientStore::ClientID;

    VoicePlayback(Context &context) noexcept;

    void OnRTPData(ClientID id, const std::vector<uint8_t> &&data) noexcept;

    void Start() noexcept;
    void Stop() noexcept;

    void SetActive(bool active) noexcept;
    void SetPlaybackDevice(ma_device_id &&device_id) noexcept;

    Playback::ClientStore& GetClientStore() noexcept;
    const Playback::ClientStore& GetClientStore() const noexcept;

private:
    void OnAudioPlayback(OutputBuffer buffer) noexcept;
    ma_device_config GetDeviceConfig() noexcept;

    AudioDevice m_device;
    Playback::ClientStore m_clients;

    std::atomic<bool> m_active = true;

    friend void playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};

};
