#include "ma_pcm_rb.hpp"

#include <utility>

#include <spdlog/spdlog.h>

namespace AbaddonClient::Audio::Miniaudio {

MaPCMRingBuffer::MaPCMRingBuffer(RingBufferPtr &&ringbuffer, uint32_t channels) noexcept :
    m_ringbuffer(std::move(ringbuffer)),
    m_channels(channels) {}


std::optional<MaPCMRingBuffer>
MaPCMRingBuffer::Create(uint32_t channels, uint32_t buffer_size_in_frames) noexcept {
    auto ringbuffer_ptr = RingBufferPtr(new ma_pcm_rb);

    const auto result = ma_pcm_rb_init(ma_format_f32, channels, buffer_size_in_frames, nullptr, nullptr, ringbuffer_ptr.get());
    if (result != MA_SUCCESS) {
        spdlog::get("voice")->error("Failed to create MaPCMRingBuffer: {}", ma_result_description(result));
        return std::nullopt;
    }

    return MaPCMRingBuffer(std::move(ringbuffer_ptr), channels);
}


void MaPCMRingBuffer::Read(OutputBuffer output) noexcept {
    const auto total_frames = output.size() / m_channels;

    uint32_t read = 0;
    uint32_t tries = 0;

    // Try twice in case of wrap around
    while (read < total_frames && tries < 2) {
        read += DoRead(output, read, total_frames);
        tries++;
    }
}

void MaPCMRingBuffer::Write(InputBuffer input) noexcept {
    const auto total_frames = input.size() / m_channels;

    uint32_t written = 0;
    uint32_t tries = 0;

    // Try twice in case of wrap around
    while (written < total_frames && tries < 2) {
        written += DoWrite(input, written, total_frames);
        tries++;
    }
}

void MaPCMRingBuffer::Clear() noexcept {
    ma_pcm_rb_reset(m_ringbuffer.get());
}

uint32_t MaPCMRingBuffer::GetAvailableReadFrames() noexcept {
    return ma_pcm_rb_available_read(m_ringbuffer.get());
}

uint32_t MaPCMRingBuffer::GetAvailableWriteFrames() noexcept {
    return ma_pcm_rb_available_write(m_ringbuffer.get());
}


uint32_t MaPCMRingBuffer::DoRead(OutputBuffer output, uint32_t read_frames, uint32_t total_frames) noexcept {
    auto frames = total_frames - read_frames;
    float* read_ptr;

    ma_pcm_rb_acquire_read(m_ringbuffer.get(), &frames, reinterpret_cast<void**>(&read_ptr));

    const auto output_ptr = output.begin() + read_frames * m_channels;
    const auto samples = frames * m_channels;

    std::copy_n(read_ptr, samples, output_ptr);

    ma_pcm_rb_commit_read(m_ringbuffer.get(), frames);

    return frames;
}

uint32_t MaPCMRingBuffer::DoWrite(InputBuffer input, uint32_t written_frames, uint32_t total_frames) noexcept {
    auto frames = total_frames - written_frames;
    float* write_ptr;

    ma_pcm_rb_acquire_write(m_ringbuffer.get(), &frames, reinterpret_cast<void**>(&write_ptr));

    const auto input_ptr = input.begin() + written_frames * m_channels;
    const auto samples = frames * m_channels;

    std::copy_n(input_ptr, samples, write_ptr);

    ma_pcm_rb_commit_write(m_ringbuffer.get(), frames);

    return frames;
}

}
