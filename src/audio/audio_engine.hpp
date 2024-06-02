#pragma once

#include <glibmm/timer.h>

#include "misc/mutex.hpp"

#include "miniaudio/ma_engine.hpp"

namespace AbaddonClient::Audio {

class AudioEngine {
public:
    AudioEngine(Context& context) noexcept;
    ~AudioEngine() noexcept;

    bool PlaySound(std::string_view file_path) noexcept;
private:
    void StartTimer() noexcept;
    void StopTimer() noexcept;

    static void TimeoutEngine(AudioEngine &engine) noexcept;

    Context &m_context;
    Mutex<std::optional<Miniaudio::MaEngine>> m_engine;

    GSource* m_timer_source;
};

}
