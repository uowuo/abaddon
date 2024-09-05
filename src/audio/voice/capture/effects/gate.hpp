#pragma once

#include <atomic>

#include "audio/utils.hpp"

namespace AbaddonClient::Audio::Voice::Capture::Effects {

class Gate {
public:
    bool PassesVAD(InputBuffer buffer, float current_peak) const noexcept;

    std::atomic<float> VADThreshold;
};

}
