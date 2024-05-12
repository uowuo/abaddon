#include "noise.hpp"

namespace AbaddonClient::Audio::Voice::Capture::Effects {

bool Noise::PassesVAD(InputBuffer buffer) noexcept {
    // Use first channel for VAD, only denoise the rest if noise suppression is enabled
    const auto prob = m_channels.Lock()[0].DenoiseChannel(buffer, 0);

    m_peak_meter.SetPeak(prob);
    denoised_first_channel = true;

    return prob > m_vad_threshold;
}

void Noise::Denoise(OutputBuffer buffer) noexcept {
    auto start = 0;
    if (denoised_first_channel) {
        denoised_first_channel = false;
        start = 1;
    }

    auto channels = m_channels.Lock();
    for (size_t channel = start; channel < channels->size(); channel++) {
        auto& channel_buffer = channels[channel];

        channel_buffer.DenoiseChannel(buffer, channel);
        channel_buffer.WriteChannel(buffer, channel);
    }
}

PeakMeter& Noise::GetPeakMeter() noexcept {
    return m_peak_meter;
}

const PeakMeter& Noise::GetPeakMeter() const noexcept {
    return m_peak_meter;
}

NoiseBuffer::NoiseBuffer() noexcept :
    m_state(RNNoisePtr(rnnoise_create(NULL), rnnoise_destroy)) {}

float NoiseBuffer::DenoiseChannel(InputBuffer buffer, size_t channel) noexcept {
    ChannelBuffer input_channel;

    // Copy interleaved samples from a specific channel
    for (size_t i = 0; i < m_channel.size(); i++) {
        const auto offset = channel + (i * CAPTURE_CHANNELS);

        // RNNoise expects samples to fall in range of 16-bit PCM, but as float. Weird
        input_channel[i] = buffer[offset] * INT16_MAX;
    }

    const auto prob = rnnoise_process_frame(m_state.get(), m_channel.data(), input_channel.data());
    return prob;
}

void NoiseBuffer::WriteChannel(OutputBuffer buffer, size_t channel) noexcept {
    for (size_t i = 0; i < m_channel.size(); i++) {
        const auto offset = channel + (i * CAPTURE_CHANNELS);
        buffer[offset] = m_channel[i] / INT16_MAX;  // Convert to f32 sample range
    }
}

}
