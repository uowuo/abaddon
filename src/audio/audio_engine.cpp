#include "audio_engine.hpp"

#include <glibmm/main.h>
#include <spdlog/spdlog.h>

namespace AbaddonClient::Audio {

AudioEngine::AudioEngine(Context &context) noexcept :
    m_context(context) {}

AudioEngine::~AudioEngine() noexcept {
    StopTimer();
}

bool AudioEngine::PlaySound(std::string_view file_path) noexcept {
    StopTimer();

    const auto result = m_engine.LockScope([this, &file_path](std::optional<Miniaudio::MaEngine> &engine) {
        if (!engine) {
            engine = Miniaudio::MaEngine::Create(m_context.GetEngineConfig());
            if (!engine) {
                return false;
            }
        }

        return engine->PlaySound(file_path);
    });

    StartTimer();
    return result;
}

void AudioEngine::StartTimer() noexcept {
    // NOTE: I am not using g_timeout_add_seconds_once here since we want to own the source and destroy it manually
    // g_source_remove throws an error on destroyed source
    m_timer_source = g_timeout_source_new_seconds(5);

    g_source_set_callback(m_timer_source, GSourceFunc(AudioEngine::TimeoutEngine), this, nullptr);
    g_source_attach(m_timer_source, Glib::MainContext::get_default()->gobj());
}

void AudioEngine::StopTimer() noexcept {
    if (m_timer_source != nullptr) {
        g_source_destroy(m_timer_source);
        g_source_unref(m_timer_source);
    }
}

void AudioEngine::TimeoutEngine(AudioEngine &engine) noexcept {
    engine.m_engine.Lock()->reset();
}

}
