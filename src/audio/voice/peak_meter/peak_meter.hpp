#pragma once

#include <atomic>

#include "audio/utils.hpp"

namespace AbaddonClient::Audio::Voice {

class PeakMeter {
public:
    PeakMeter() noexcept;

    void UpdatePeak(InputBuffer buffer) noexcept;

    void SetPeak(float peak) noexcept;
    float GetPeak() const noexcept;
private:
    bool Decay() noexcept;

    std::atomic<float> m_peak = 0;
};

};
