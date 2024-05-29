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
    m_ok = true;

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
}

void AudioManager::FeedMeOpus(uint32_t ssrc, std::vector<uint8_t> &&data) {
    m_voice->GetPlayback().OnRTPData(ssrc, std::move(data));
}


void AudioManager::SetPlaybackDevice(const Gtk::TreeModel::iterator &iter) {
    auto device_id = m_devices.GetPlaybackDeviceIDFromModel(iter);
    if (!device_id) {
        spdlog::get("audio")->error("Requested ID from iterator is invalid");
        return;
    }

    m_voice->GetPlayback().SetPlaybackDevice(*device_id);
}

void AudioManager::SetCaptureDevice(const Gtk::TreeModel::iterator &iter) {
    auto device_id = m_devices.GetCaptureDeviceIDFromModel(iter);
    if (!device_id) {
        spdlog::get("audio")->error("Requested ID from iterator is invalid");
        return;
    }

    m_voice->GetCapture().SetCaptureDevice(*device_id);
}

void AudioManager::SetCapture(bool capture) {
    m_voice->GetCapture().SetActive(capture);
}

void AudioManager::SetPlayback(bool playback) {
    m_voice->GetPlayback().SetActive(playback);
}

void AudioManager::SetCaptureGate(double gate) {
    m_voice->GetCapture().GetEffects().GetGate().VADThreshold = gate;
}

void AudioManager::SetCaptureGain(double gain) {
    m_voice->GetCapture().Gain = gain;
}

double AudioManager::GetCaptureGate() const noexcept {
    return m_voice->GetCapture().GetEffects().GetGate().VADThreshold;
}

double AudioManager::GetCaptureGain() const noexcept {
    return m_voice->GetCapture().Gain;
}

void AudioManager::SetMuteSSRC(uint32_t ssrc, bool mute) {
    m_voice->GetPlayback().GetClientStore().SetClientMute(ssrc, mute);
}

void AudioManager::SetVolumeSSRC(uint32_t ssrc, double volume) {
   m_voice->GetPlayback().GetClientStore().SetClientVolume(ssrc, volume);
}

double AudioManager::GetVolumeSSRC(uint32_t ssrc) const {
    return m_voice->GetPlayback().GetClientStore().GetClientVolume(ssrc);
}

void AudioManager::SetEncodingApplication(int application) {
    const auto _application = static_cast<AbaddonClient::Audio::Voice::Opus::OpusEncoder::EncodingApplication>(application);
    m_voice->GetCapture().GetEncoder()->value().SetEncodingApplication(_application);
}

int AudioManager::GetEncodingApplication() {
    const auto application = m_voice->GetCapture().GetEncoder()->value().GetEncodingApplication();
    return static_cast<int>(application);
}

void AudioManager::SetSignalHint(int signal) {
    const auto _signal = static_cast<AbaddonClient::Audio::Voice::Opus::OpusEncoder::SignalHint>(signal);
    m_voice->GetCapture().GetEncoder()->value().SetSignalHint(_signal);
}

int AudioManager::GetSignalHint() {
    const auto hint = m_voice->GetCapture().GetEncoder()->value().GetSignalHint();
    return static_cast<int>(hint);
}

void AudioManager::SetBitrate(int bitrate) {
    m_voice->GetCapture().GetEncoder()->value().SetBitrate(bitrate);
}

int AudioManager::GetBitrate() {
    return m_voice->GetCapture().GetEncoder()->value().GetBitrate();
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

double AudioManager::GetCaptureVolumeLevel() const noexcept {
    return m_voice->GetCapture().GetPeakMeter().GetPeak();
}

double AudioManager::GetSSRCVolumeLevel(uint32_t ssrc) const noexcept {
    return m_voice->GetPlayback().GetClientStore().GetClientPeakVolume(ssrc);
}

AudioDevices &AudioManager::GetDevices() {
    return m_devices;
}

uint32_t AudioManager::GetRTPTimestamp() const noexcept {
    return m_voice->GetCapture().GetRTPTimestamp();
}

void AudioManager::SetVADMethod(const std::string &method) {
    spdlog::get("audio")->debug("Setting VAD method to {}", method);
    m_voice->GetCapture().GetEffects().SetVADMethod(method);
}

void AudioManager::SetVADMethod(VADMethod method) {
    const auto method_int = static_cast<int>(method);
    spdlog::get("audio")->debug("Setting VAD method to enum {}", method_int);

    m_voice->GetCapture().GetEffects().SetVADMethod(method_int);
}

AudioManager::VADMethod AudioManager::GetVADMethod() const {
    const auto method = m_voice->GetCapture().GetEffects().GetVADMethod();
    return static_cast<VADMethod>(method);
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

#ifdef WITH_RNNOISE
float AudioManager::GetCurrentVADProbability() const {
    return m_voice->GetCapture().GetEffects().GetNoise().GetPeakMeter().GetPeak();
}

double AudioManager::GetRNNProbThreshold() const {
    return m_voice->GetCapture().GetEffects().GetNoise().VADThreshold;
}

void AudioManager::SetRNNProbThreshold(double value) {
    m_voice->GetCapture().GetEffects().GetNoise().VADThreshold = value;
}

void AudioManager::SetSuppressNoise(bool value) {
    m_voice->GetCapture().SuppressNoise = value;
}

bool AudioManager::GetSuppressNoise() const {
    return m_voice->GetCapture().SuppressNoise;
}
#endif

void AudioManager::SetMixMono(bool value) {
    m_voice->GetCapture().MixMono = value;
}

bool AudioManager::GetMixMono() const {
    return m_voice->GetCapture().MixMono;
}

AbaddonClient::Audio::Voice::VoiceCapture::CaptureSignal AudioManager::signal_opus_packet() {
    return m_voice->GetCapture().GetCaptureSignal();
}

#endif
