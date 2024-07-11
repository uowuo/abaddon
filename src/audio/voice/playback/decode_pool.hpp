#pragma once

#include "audio/voice/opus/opus_decoder.hpp"
#include "voice_buffer.hpp"

#include "misc/threadpool.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

using SharedDecoder = std::shared_ptr<Mutex<Opus::OpusDecoder>>;
using SharedBuffer = std::shared_ptr<VoiceBuffer>;

class DecodePool {
public:
    DecodePool() noexcept;

    struct DecodeData {
        std::vector<uint8_t> opus;

        SharedDecoder decoder;
        SharedBuffer buffer;
    };

    void Decode(DecodeData &&decode_data) noexcept;

    void AddDecoder() noexcept;
    void RemoveDecoder() noexcept;
    void ClearDecoders() noexcept;

    size_t GetDecoderCount() const noexcept;

private:
    static void DecodeThread(Channel<std::variant<DecodeData, Terminate>> &channel) noexcept;
    static void OnDecodeMessage(DecodeData &&decode_data) noexcept;

    using Pool = ThreadPool<DecodeData, decltype(&DecodePool::DecodeThread)>;

    Pool m_pool;
};

}
