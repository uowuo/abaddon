#pragma once

#include "audio/utils.hpp"

namespace AbaddonClient::Audio::Voice {

class PeakMeter {
public:
    void UpdatePeak(InputBuffer buffer) noexcept;
    void Decay() noexcept;

    void SetPeak(float peak) noexcept;
    float GetPeak() const noexcept;
private:
    std::atomic<float> m_peak = 0;
};

};
