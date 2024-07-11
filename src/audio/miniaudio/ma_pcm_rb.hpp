#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include <miniaudio.h>

#include "audio/utils.hpp"

namespace AbaddonClient::Audio::Miniaudio {

class MaPCMRingBuffer {
public:
    static std::optional<MaPCMRingBuffer> Create(uint32_t channels, uint32_t buffer_size_in_frames) noexcept;

    void Read(OutputBuffer output) noexcept;
    void Write(InputBuffer input) noexcept;
    void Clear() noexcept;

    uint32_t GetAvailableReadFrames() noexcept;
    uint32_t GetAvailableWriteFrames() noexcept;
private:
    uint32_t DoRead(OutputBuffer output, uint32_t read_frames, uint32_t total_frames) noexcept;
    uint32_t DoWrite(InputBuffer input, uint32_t written_frames, uint32_t total_frames) noexcept;

    struct RingBufferDeleter {
        void operator()(ma_pcm_rb* ptr) noexcept {
            ma_pcm_rb_uninit(ptr);
        }
    };

    using RingBufferPtr = std::unique_ptr<ma_pcm_rb, RingBufferDeleter>;
    MaPCMRingBuffer(RingBufferPtr &&ringbuffer, uint32_t channel) noexcept;

    RingBufferPtr m_ringbuffer;

    const uint32_t m_channels;
};

}
