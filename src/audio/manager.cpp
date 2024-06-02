#ifdef WITH_MINIAUDIO
// clang-format off

#ifdef _WIN32
    #include <winsock2.h>
#endif

#include "manager.hpp"
#include "abaddon.hpp"
#include <glibmm/main.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <miniaudio.h>
#include <opus.h>
#include <cstring>
// clang-format on

void mgr_log_callback(void *pUserData, ma_uint32 level, const char *pMessage) {
    auto *log = static_cast<spdlog::logger *>(pUserData);

    gchar *msg = g_strstrip(g_strdup(pMessage));

    switch (level) {
        case MA_LOG_LEVEL_DEBUG:
            log->debug(msg);
            break;
        case MA_LOG_LEVEL_INFO:
            log->info(msg);
            break;
        case MA_LOG_LEVEL_WARNING:
            log->warn(msg);
            break;
        case MA_LOG_LEVEL_ERROR:
            log->error(msg);
        default:
            break;
    }

    g_free(msg);
}

AudioManager::AudioManager(const Glib::ustring &backends_string, DiscordClient &discord)
    : m_log(spdlog::stdout_color_mt("miniaudio")),
      m_ma_log(AbaddonClient::Audio::Miniaudio::MaLog::Create())
{
    auto ctx_cfg = ma_context_config_init();

    if (m_ma_log) {
        m_ma_log->RegisterCallback(ma_log_callback_init(mgr_log_callback, m_log.get()));
        ctx_cfg.pLog = &m_ma_log->GetInternal();
    }

    std::vector<ma_backend> backends;
    if (!backends_string.empty()) {
        spdlog::get("audio")->debug("Using backends list: {}", std::string(backends_string));
        backends = ParseBackendsList(backends_string);
    }

    m_context = AbaddonClient::Audio::Context::Create(std::move(ctx_cfg), backends);

    if (m_context) {
        Enumerate();

#if WITH_VOICE
        m_voice.emplace(*m_context, discord);
#endif
    }

    Glib::signal_timeout().connect(sigc::mem_fun(*this, &AudioManager::DecayVolumeMeters), 40);

    m_ok = true;
}

void AudioManager::Enumerate() {
    ma_device_info *pPlaybackDeviceInfo;
    ma_uint32 playbackDeviceCount;
    ma_device_info *pCaptureDeviceInfo;
    ma_uint32 captureDeviceCount;

    spdlog::get("audio")->debug("Enumerating devices");

    const auto playback_devices = m_context->GetPlaybackDevices();
    const auto capture_devices = m_context->GetCaptureDevices();

    spdlog::get("audio")->info("Found {} playback devices and {} capture devices", playback_devices.size(), capture_devices.size());

    m_devices.SetDevices(
        playback_devices.data(),
        playback_devices.size(),
        capture_devices.data(),
        capture_devices.size()
    );
}

bool AudioManager::DecayVolumeMeters() {
    m_voice->GetCapture().GetPeakMeter().Decay();
    m_voice->GetPlayback().GetClientStore().DecayPeakMeters();

    return true;
}

bool AudioManager::OK() const {
    return m_ok;
}

AudioDevices &AudioManager::GetDevices() {
    return m_devices;
}

std::vector<ma_backend> AudioManager::ParseBackendsList(const Glib::ustring &list) {
    auto regex = Glib::Regex::create(";");
    const std::vector<Glib::ustring> split = regex->split(list);

    std::vector<ma_backend> backends;
    for (const auto &s : split) {
        if (s == "wasapi") backends.push_back(ma_backend_wasapi);
        else if (s == "dsound") backends.push_back(ma_backend_dsound);
        else if (s == "winmm") backends.push_back(ma_backend_winmm);
        else if (s == "coreaudio") backends.push_back(ma_backend_coreaudio);
        else if (s == "sndio") backends.push_back(ma_backend_sndio);
        else if (s == "audio4") backends.push_back(ma_backend_audio4);
        else if (s == "oss") backends.push_back(ma_backend_oss);
        else if (s == "pulseaudio") backends.push_back(ma_backend_pulseaudio);
        else if (s == "alsa") backends.push_back(ma_backend_alsa);
        else if (s == "jack") backends.push_back(ma_backend_jack);
    }
    backends.push_back(ma_backend_null);

    return backends;
}

#ifdef WITH_VOICE

AbaddonClient::Audio::VoiceAudio& AudioManager::GetVoice() noexcept {
    return *m_voice;
}

const AbaddonClient::Audio::VoiceAudio& AudioManager::GetVoice() const noexcept {
    return *m_voice;
}

#endif


#endif
