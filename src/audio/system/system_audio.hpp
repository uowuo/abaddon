#pragma once

#include "audio/audio_engine.hpp"

namespace AbaddonClient::Audio {

class SystemAudio {
public:
    enum SystemSound {
#ifdef ENABLE_NOTIFICATION_SOUNDS
        NOTIFICATION_SOUND
#endif
    };

    SystemAudio(Context &context) noexcept;

    void PlaySound(SystemSound sound) noexcept;

private:
    AudioEngine m_engine;
};

}
