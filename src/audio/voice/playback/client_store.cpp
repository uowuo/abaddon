#include "client_store.hpp"
#include "constants.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

void ClientStore::AddClient(ClientID id) noexcept {
    auto clients = m_clients.Lock();

    if (clients->find(id) != clients->end()) {
        spdlog::get("voice")->error("Tried to add an existing client: {}", id);
        return;
    };

    auto settings = Opus::OpusDecoder::DecoderSettings {};
    settings.sample_rate = RTP_SAMPLE_RATE;
    settings.channels = RTP_CHANNELS;

    auto decoder =  Opus::OpusDecoder::Create(settings);
    if (!decoder) {
        return;
    }

    auto buffer = VoiceBuffer::Create(RTP_CHANNELS, RTP_OPUS_FRAME_SIZE * 4, 1024);
    if (!buffer) {
        return;
    }

    m_decode_pool.AddDecoder();

    clients->emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(std::move(*decoder), std::move(*buffer), m_decode_pool));
}

void ClientStore::RemoveClient(ClientID id) noexcept {
    auto clients = m_clients.Lock();
    clients->erase(id);

    // Keep the decoder count below client count
    // Doesn't make sense to have more
    if (m_decode_pool.GetDecoderCount() > clients->size()) {
        m_decode_pool.RemoveDecoder();
    }
}

void ClientStore::Clear() noexcept {
    m_decode_pool.ClearDecoders();
    m_clients.Lock()->clear();
}

void ClientStore::DecodeFromRTP(ClientID id, const std::vector<uint8_t> &&data) noexcept {
    auto clients = m_clients.Lock();
    auto client = clients->find(id);

    if (client == clients->end()) {
        spdlog::get("voice")->error("Tried to decode Opus data for missing client: {}", id);
        return;
    }

    client->second.DecodeFromRTP(std::move(data));
}

void ClientStore::WriteMixed(OutputBuffer buffer) noexcept {
    auto clients = m_clients.Lock();

    // Reusing per client buffer,
    // miniaudio doesn't provide a way to know the maximum buffer size in advance
    // so we have to resize it dynamically.
    // This shouldn't have a big performance hit as the capacity will settle on some value eventually

    for (auto& [it, client] : clients) {
        m_client_buffer.resize(buffer.size());

        client.WriteAudio(m_client_buffer);
        AudioUtils::MixBuffers(m_client_buffer, buffer);

        m_client_buffer.clear();
    }
}

void ClientStore::DecayPeakMeters() noexcept {
    auto clients = m_clients.Lock();

    for (auto& [_, client] : clients) {
        client.GetPeakMeter().Decay();
    }
}

void ClientStore::SetClientVolume(const ClientID id, const float volume) noexcept {
    auto clients = m_clients.Lock();
    auto client = clients->find(id);

    if (client != clients->end()) {
        client->second.m_volume = volume;
    }
}

void ClientStore::ClearAllBuffers() noexcept {
    auto clients = m_clients.Lock();

    for (auto& [_, client] : clients) {
        client.ClearBuffer();
    }
}

void ClientStore::SetClientMute(const ClientID id, const bool muted) noexcept {
    auto clients = m_clients.Lock();
    auto client = clients->find(id);

    if (client != clients->end()) {
        client->second.SetMuted(muted);
    }
}

float ClientStore::GetClientVolume(const ClientID id) const noexcept {
    const auto clients = m_clients.Lock();
    const auto client = clients->find(id);

    if (client != clients->end()) {
        return client->second.m_volume;
    }

    return 1.0;
}

bool ClientStore::GetClientMute(const ClientID id) const noexcept {
    const auto clients = m_clients.Lock();
    const auto client = clients->find(id);

    if (client != clients->end()) {
        return client->second.GetMuted();
    }

    return false;
}

float ClientStore::GetClientPeakVolume(const ClientID id) const noexcept {
    const auto clients = m_clients.Lock();
    const auto client = clients->find(id);

    if (client != clients->end()) {
        return client->second.GetPeakMeter().GetPeak();
    }

    return 0.0f;
}

}
