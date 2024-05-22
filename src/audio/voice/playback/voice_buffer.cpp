#include "voice_buffer.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

VoiceBuffer::VoiceBuffer(Miniaudio::MaPCMRingBuffer &&ringbuffer, uint32_t buffer_frames) noexcept :
    m_ringbuffer(std::move(ringbuffer)),
    m_buffer_frames(buffer_frames) {}

std::optional<VoiceBuffer> VoiceBuffer::Create(uint32_t channels, uint32_t buffer_size_in_frames, uint32_t buffer_frames) noexcept {
    auto ringbuffer = Miniaudio::MaPCMRingBuffer::Create(channels, buffer_size_in_frames);

    if (!ringbuffer) {
        return std::nullopt;
    }

    return VoiceBuffer(std::move(*ringbuffer), buffer_frames);
}

void VoiceBuffer::Read(OutputBuffer output) noexcept {
    if (m_ringbuffer.GetAvailableReadFrames() < m_buffer_frames) {
        return;
    }

    m_ringbuffer.Read(output);
}

void VoiceBuffer::Write(InputBuffer input) noexcept {
    m_ringbuffer.Write(input);
}

void VoiceBuffer::Clear() noexcept {
    m_ringbuffer.Clear();
}

}
