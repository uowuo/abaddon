#pragma once

#include "audio/miniaudio/ma_pcm_rb.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

class VoiceBuffer {
public:
    static std::optional<VoiceBuffer> Create(uint32_t channels, uint32_t buffer_size_in_frames, uint32_t buffer_frames) noexcept;

    void Read(OutputBuffer output) noexcept;
    void Write(InputBuffer input) noexcept;
    void Clear() noexcept;

private:
    VoiceBuffer(Miniaudio::MaPCMRingBuffer &&ringbuffer, uint32_t buffer_frames) noexcept;

    Miniaudio::MaPCMRingBuffer m_ringbuffer;
    const uint32_t m_buffer_frames;
};

}
