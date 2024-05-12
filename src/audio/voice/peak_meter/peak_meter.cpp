#include "peak_meter.hpp"

namespace AbaddonClient::Audio::Voice {

void PeakMeter::UpdatePeak(InputBuffer buffer) noexcept {
    // Cache to prevent atomic operations in the loop
    float peak = m_peak;

    for (const auto& sample: buffer) {
        peak = std::max(peak, std::abs(sample));
    }

    m_peak = peak;
}

void PeakMeter::Decay() noexcept {
    m_peak = std::max(m_peak - 0.05f, 0.0f);
}

void PeakMeter::SetPeak(float peak) noexcept {
    m_peak = std::max(m_peak.load(), peak);
}

float PeakMeter::GetPeak() const noexcept {
    return m_peak;
}


}
