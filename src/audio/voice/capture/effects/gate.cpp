#include "gate.hpp"

namespace AbaddonClient::Audio::Voice::Capture::Effects {

bool Gate::PassesVAD(InputBuffer buffer, float current_peak) const noexcept {
    return current_peak > VADThreshold;
}

}
