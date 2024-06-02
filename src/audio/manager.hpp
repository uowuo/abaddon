#pragma once
#ifdef WITH_MINIAUDIO
// clang-format off

#include <gtkmm/treemodel.h>
#include <vector>
#include <miniaudio.h>
#include <opus.h>
#include <sigc++/sigc++.h>
#include <spdlog/spdlog.h>

#include "devices.hpp"

#ifdef WITH_VOICE
#include "voice/voice_audio.hpp"
#endif

#include "miniaudio/ma_log.hpp"

// clang-format on

class AudioManager {
public:
    AudioManager(const Glib::ustring &backends_string, DiscordClient &discord);

#if WITH_VOICE
    AbaddonClient::Audio::VoiceAudio& GetVoice() noexcept;
    const AbaddonClient::Audio::VoiceAudio& GetVoice() const noexcept;
#endif
    bool OK() const;

    AudioDevices &GetDevices();

private:
    void Enumerate();
    static std::vector<ma_backend> ParseBackendsList(const Glib::ustring &list);

    bool DecayVolumeMeters();

    AudioDevices m_devices;

    std::optional<AbaddonClient::Audio::Context> m_context;
    std::optional<AbaddonClient::Audio::Miniaudio::MaLog> m_ma_log;

    std::shared_ptr<spdlog::logger> m_log;

#ifdef WITH_VOICE
    std::optional<AbaddonClient::Audio::VoiceAudio> m_voice;
#endif

    bool m_ok = false;
};
#endif
