#pragma once

#include "audio/utils.hpp"

#include "effects/gate.hpp"

#ifdef  WITH_RNNOISE
#include "effects/noise.hpp"
#endif

namespace AbaddonClient::Audio::Voice::Capture {

enum class VADMethod {
    Gate,
#ifdef WITH_RNNOISE
    RNNoise
#endif
};

class VoiceEffects {
public:
    bool PassesVAD(InputBuffer buffer, float current_volume) noexcept;
    void Denoise(OutputBuffer buffer) noexcept;

    void SetCurrentThreshold(float threshold) noexcept;
    float GetCurrentThreshold() const noexcept;

    void SetVADMethod(const std::string &method) noexcept;
    void SetVADMethod(VADMethod method) noexcept;
    VADMethod GetVADMethod() const noexcept;

    Effects::Gate& GetGate() noexcept;
    const Effects::Gate& GetGate() const noexcept;

#ifdef WITH_RNNOISE
    Effects::Noise& GetNoise() noexcept;
    const Effects::Noise& GetNoise() const noexcept;
#endif

    VADMethod m_vad_method;
private:
    Effects::Gate m_gate;

#ifdef WITH_RNNOISE
    Effects::Noise m_noise;
#endif
};

};
