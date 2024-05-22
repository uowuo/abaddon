#pragma once

#include <miniaudio.h>

namespace AbaddonClient::Audio::Voice::Playback {

class VoiceBuffer {
public:
    static std::optional<VoiceBuffer> Create(uint32_t channels, uint32_t buffer_size_in_frames, uint32_t buffer_frames) noexcept;

    void Read(OutputBuffer output) noexcept;
    void Write(InputBuffer input) noexcept;
    void Clear() noexcept;

private:
    struct RingBufferDeleter {
        void operator()(ma_pcm_rb* ptr) noexcept {
            ma_pcm_rb_uninit(ptr);
        }
    };

    using RingBufferPtr = std::unique_ptr<ma_pcm_rb, RingBufferDeleter>;
    VoiceBuffer(RingBufferPtr &&ringbuffer, uint32_t channels, uint32_t buffer_frames) noexcept;

    RingBufferPtr m_ringbuffer;

    uint32_t m_channels;
    uint32_t m_buffer_frames;

    bool moved = false;
};

}
