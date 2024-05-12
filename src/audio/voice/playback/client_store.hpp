#pragma once

#include "audio/utils.hpp"

#include "client.hpp"
#include "decode_pool.hpp"

// This needs to be forward declared for friend declaration
namespace AbaddonClient::Audio::Voice {
    class VoicePlayback;
}

namespace AbaddonClient::Audio::Voice::Playback {


class ClientStore {
public:
    using ClientID = uint32_t;

    void AddClient(const ClientID id) noexcept;
    void RemoveClient(const ClientID id) noexcept;
    void Clear() noexcept;

    void DecayPeakMeters() noexcept;
    void ClearAllBuffers() noexcept;

    void SetClientVolume(const ClientID id, const float volume) noexcept;
    void SetClientMute(const ClientID id, const bool muted) noexcept;

    float GetClientVolume(const ClientID id) const noexcept;
    bool GetClientMute(const ClientID id) const noexcept;

    float GetClientPeakVolume(const ClientID id) const noexcept;

private:
    using ClientMap = std::unordered_map<ClientID, Client>;

    // Keep these two private and expose through VoicePlayback
    void DecodeFromRTP(const ClientID id, const std::vector<uint8_t> &&data) noexcept;
    void WriteMixed(OutputBuffer buffer) noexcept;

    DecodePool m_decode_pool;
    Mutex<ClientMap> m_clients;

    std::vector<float> m_client_buffer;

    friend Voice::VoicePlayback;
};

}
