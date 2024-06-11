#pragma once

// clang-format off

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "devices.hpp"

#include "audio/context.hpp"

#ifdef WITH_VOICE
#include "voice/voice_audio.hpp"
#endif

#include "system/system_audio.hpp"

#include "miniaudio/ma_log.hpp"

// clang-format on

class AudioManager {
public:
    AudioManager(const Glib::ustring &backends_string, DiscordClient &discord);

    AudioDevices &GetDevices();

    AbaddonClient::Audio::Context& GetContext() noexcept;
    const AbaddonClient::Audio::Context& GetContext() const noexcept;

    AbaddonClient::Audio::SystemAudio& GetSystem() noexcept;
    const AbaddonClient::Audio::SystemAudio& GetSystem() const noexcept;

#if WITH_VOICE
    AbaddonClient::Audio::VoiceAudio& GetVoice() noexcept;
    const AbaddonClient::Audio::VoiceAudio& GetVoice() const noexcept;
#endif

    bool OK() const;

private:
    void Enumerate();
    static std::vector<ma_backend> ParseBackendsList(const Glib::ustring &list);

    AudioDevices m_devices;

    std::optional<AbaddonClient::Audio::Context> m_context;
    std::optional<AbaddonClient::Audio::Miniaudio::MaLog> m_ma_log;

    std::shared_ptr<spdlog::logger> m_log;

#ifdef WITH_VOICE
    std::optional<AbaddonClient::Audio::VoiceAudio> m_voice;
#endif

    std::optional<AbaddonClient::Audio::SystemAudio> m_system;

    bool m_ok = false;
};
