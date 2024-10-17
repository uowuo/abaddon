#include "decode_pool.hpp"
#include "constants.hpp"

#include <array>

namespace AbaddonClient::Audio::Voice::Playback {



DecodePool::DecodePool() noexcept :
    m_pool(Pool(&DecodePool::DecodeThread, 20)) {}

void DecodePool::Decode(DecodeData &&decode_data) noexcept {
    m_pool.SendToPool(std::move(decode_data));
}

void DecodePool::AddDecoder() noexcept {
    m_pool.AddThread();
}

void DecodePool::RemoveDecoder() noexcept {
    m_pool.RemoveThread();
}

void DecodePool::ClearDecoders() noexcept {
    m_pool.Clear();
}

size_t DecodePool::GetDecoderCount() const noexcept {
    return m_pool.GetThreadCount();
}

void DecodePool::DecodeThread(Channel<Pool::ThreadMessage> &channel) noexcept {
    while (true) {
        auto message = channel.Recv();
        if (std::holds_alternative<Terminate>(message)) {
            break;
        }

        auto&& decode_message = std::get<DecodeData>(message);
        OnDecodeMessage(std::move(decode_message));
    }
}

void DecodePool::OnDecodeMessage(DecodeData &&message) noexcept {
    auto& [opus, decoder, buffer] = message;

    static std::array<float, RTP_OPUS_MAX_BUFFER_SIZE> pcm;

    int frames = decoder->Lock()->Decode(opus, pcm, RTP_OPUS_FRAME_SIZE);
    if (frames < 0) {
        return;
    }

    // Take only what we've got from the decoder
    auto pcm_written = OutputBuffer(pcm.data(), frames * RTP_CHANNELS);
    buffer->Write(pcm_written);
}

}
