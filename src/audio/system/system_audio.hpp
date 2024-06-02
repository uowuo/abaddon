#pragma once

#include "audio/miniaudio/ma_engine.hpp"

namespace AbaddonClient::Audio {

class SystemAudio {
public:
    enum SystemSound {
#ifdef ENABLE_NOTIFICATION_SOUNDS
        NOTIFICATION_SOUND
#endif
    };

    SystemAudio(Miniaudio::MaEngine &&engine) noexcept;

    void PlaySound(SystemSound sound) noexcept;
private:
    Miniaudio::MaEngine m_engine;
};

}
