#pragma once

#include "misc/mutex.hpp"

#include "audio/voice/capture/constants.hpp"
#include "audio/voice/peak_meter/peak_meter.hpp"

#include <rnnoise.h>

namespace AbaddonClient::Audio::Voice::Capture::Effects {

class NoiseBuffer {
public:
    NoiseBuffer() noexcept;

    float DenoiseChannel(InputBuffer buffer, size_t channel) noexcept;
    void WriteChannel(OutputBuffer buffer, size_t channel) noexcept;

private:
    using RNNoisePtr = std::unique_ptr<DenoiseState, decltype(&rnnoise_destroy)>;
    using ChannelBuffer = std::array<float, CAPTURE_FRAME_SIZE>;

    RNNoisePtr m_state;
    ChannelBuffer m_channel;
};

class Noise {
public:
    bool PassesVAD(InputBuffer buffer) noexcept;
    void Denoise(OutputBuffer buffer) noexcept;

    PeakMeter& GetPeakMeter() noexcept;
    const PeakMeter& GetPeakMeter() const noexcept;

    std::atomic<float> VADThreshold = 0.5;
private:
    using NoiseArray = std::array<NoiseBuffer, CAPTURE_CHANNELS>;

    Mutex<NoiseArray> m_channels;
    PeakMeter m_peak_meter;

    bool m_denoised_first_channel;
};

};
