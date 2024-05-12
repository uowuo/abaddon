#pragma once

#include <miniaudio.h>

namespace AbaddonClient::Audio::Voice::Playback {

class VoiceBuffer {
public:
    using RingBufferPtr = std::unique_ptr<ma_pcm_rb, decltype(&ma_pcm_rb_uninit)>;

    VoiceBuffer(RingBufferPtr &&ringbuffer, uint32_t channels, uint32_t buffer_frames) noexcept;
    static std::optional<VoiceBuffer> Create(uint32_t channels, uint32_t buffer_size_in_frames, uint32_t buffer_frames) noexcept;

    void Read(OutputBuffer output) noexcept;
    void Write(InputBuffer input) noexcept;
    void Clear() noexcept;

private:


    RingBufferPtr m_ringbuffer;

    uint32_t m_channels;
    uint32_t m_buffer_frames;

    bool moved = false;
};

}
