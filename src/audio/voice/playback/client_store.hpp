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

    void AddClient(ClientID id) noexcept;
    void RemoveClient(ClientID id) noexcept;
    void Clear() noexcept;

    void ClearAllBuffers() noexcept;

    void SetClientVolume(ClientID id, float volume) noexcept;
    void SetClientMute(ClientID id, bool muted) noexcept;

    float GetClientVolume(ClientID id) const noexcept;
    bool GetClientMute(ClientID id) const noexcept;

    float GetClientPeakVolume(ClientID id) const noexcept;

private:
    using ClientMap = std::unordered_map<ClientID, Client>;

    // Keep these two private and expose through VoicePlayback
    void DecodeFromRTP(ClientID id, std::vector<uint8_t> &&data) noexcept;
    void WriteMixed(OutputBuffer buffer) noexcept;

    DecodePool m_decode_pool;
    Mutex<ClientMap> m_clients;

    std::vector<float> m_client_buffer;

    friend Voice::VoicePlayback;
};

}
