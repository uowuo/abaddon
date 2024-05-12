#include "voice_effects.hpp"

namespace AbaddonClient::Audio::Voice::Capture {

bool VoiceEffects::PassesVAD(InputBuffer buffer, float current_peak) noexcept {
    switch (m_vad_method) {
        case VADMethod::Gate: {
            return m_gate.PassesVAD(buffer, current_peak);
        }
#ifdef WITH_RNNOISE
        case VADMethod::RNNoise: {
            return m_noise.PassesVAD(buffer);
        }
#endif
    }

    return true;
};

void VoiceEffects::Denoise(OutputBuffer buffer) noexcept {
#ifdef WITH_RNNOISE
    m_noise.Denoise(buffer);
#endif
}

void VoiceEffects::SetVADMethod(const std::string &method) noexcept {
    if (method == "gate") {
        m_vad_method = VADMethod::Gate;
    }
#ifdef WITH_RNNOISE
    else if (method == "rnnoise") {
        m_vad_method = VADMethod::RNNoise;
    }
#endif
    else {
        spdlog::get("voice")->error("Tried to set non-existent VAD method: {}", method);
    }
}

void VoiceEffects::SetVADMethod(int method) noexcept {
    m_vad_method = static_cast<VADMethod>(method);
}

int VoiceEffects::GetVADMethod() const noexcept {
    return static_cast<int>(m_vad_method);
}

Effects::Gate& VoiceEffects::GetGate() noexcept {
    return m_gate;
}

const Effects::Gate& VoiceEffects::GetGate() const noexcept {
    return m_gate;
}

#ifdef WITH_RNNOISE

Effects::Noise& VoiceEffects::GetNoise() noexcept {
    return m_noise;
}

const Effects::Noise& VoiceEffects::GetNoise() const noexcept {
    return m_noise;
}

#endif

}
