#include "peak_meter.hpp"

namespace AbaddonClient::Audio::Voice {

PeakMeter::PeakMeter() noexcept {
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &PeakMeter::Decay), 40);
}

void PeakMeter::UpdatePeak(InputBuffer buffer) noexcept {
    // Cache to prevent atomic operations in the loop
    float peak = m_peak;

    for (const auto& sample: buffer) {
        peak = std::max(peak, std::abs(sample));
    }

    m_peak = peak;
}

void PeakMeter::SetPeak(float peak) noexcept {
    m_peak = std::max(m_peak.load(), peak);
}

float PeakMeter::GetPeak() const noexcept {
    return m_peak;
}

bool PeakMeter::Decay() noexcept {
    m_peak = std::max(m_peak - 0.05f, 0.0f);
    return true;
}

}
