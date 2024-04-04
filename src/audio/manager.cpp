#ifdef WITH_VOICE
// clang-format off

#ifdef _WIN32
    #include <winsock2.h>
#endif

#include "manager.hpp"
#include "abaddon.hpp"
#include <array>
#include <glibmm/main.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <miniaudio.h>
#include <opus.h>
#include <cstring>
// clang-format on

const uint8_t *StripRTPExtensionHeader(const uint8_t *buf, int num_bytes, size_t &outlen) {
    if (buf[0] == 0xbe && buf[1] == 0xde && num_bytes > 4) {
        uint64_t offset = 4 + 4 * ((buf[2] << 8) | buf[3]);

        outlen = num_bytes - offset;
        return buf + offset;
    }
    outlen = num_bytes;
    return buf;
}

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    AudioManager *mgr = reinterpret_cast<AudioManager *>(pDevice->pUserData);
    if (mgr == nullptr) return;
    std::lock_guard<std::mutex> _(mgr->m_mutex);

    auto *pOutputF32 = static_cast<float *>(pOutput);
    for (auto &[ssrc, pair] : mgr->m_sources) {
        double volume = 1.0;
        if (const auto vol_it = mgr->m_volume_ssrc.find(ssrc); vol_it != mgr->m_volume_ssrc.end()) {
            volume = vol_it->second;
        }
        auto &buf = pair.first;
        const size_t n = std::min(static_cast<size_t>(buf.size()), static_cast<size_t>(frameCount * 2ULL));
        for (size_t i = 0; i < n; i++) {
            pOutputF32[i] += volume * buf[i] / 32768.F;
        }
        buf.erase(buf.begin(), buf.begin() + n);
    }
}

void capture_data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    auto *mgr = reinterpret_cast<AudioManager *>(pDevice->pUserData);
    if (mgr == nullptr) return;

    mgr->OnCapturedPCM(static_cast<const int16_t *>(pInput), frameCount);

    /*
     * You can simply increment it by 480 in UDPSocket::SendEncrypted but this is wrong
     * The timestamp is supposed to be strictly linear eg. if there's discontinuous
     * transmission for 1 second then the timestamp should be 48000 greater than the
     * last packet. So it's incremented here because this is fired 100x per second
     * and is always called in sync with UDPSocket::SendEncrypted
     */
    mgr->m_rtp_timestamp += 480;
}

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

AudioManager::AudioManager(const Glib::ustring &backends_string)
    : m_log(spdlog::stdout_color_mt("miniaudio")) {
    m_ok = true;

    ma_log_init(nullptr, &m_ma_log);
    ma_log_register_callback(&m_ma_log, ma_log_callback_init(mgr_log_callback, m_log.get()));

#ifdef WITH_RNNOISE
    RNNoiseInitialize();
#endif

    int err;
    m_encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK) {
        spdlog::get("audio")->error("failed to initialize opus encoder: {}", err);
        m_ok = false;
        return;
    }
    opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(64000));

    auto ctx_cfg = ma_context_config_init();
    ctx_cfg.pLog = &m_ma_log;

    ma_backend *pBackends = nullptr;
    ma_uint32 backendCount = 0;

    std::vector<ma_backend> backends_vec;
    if (!backends_string.empty()) {
        spdlog::get("audio")->debug("Using backends list: {}", std::string(backends_string));
        backends_vec = ParseBackendsList(backends_string);
        pBackends = backends_vec.data();
        backendCount = static_cast<ma_uint32>(backends_vec.size());
    }

    if (ma_context_init(pBackends, backendCount, &ctx_cfg, &m_context) != MA_SUCCESS) {
        spdlog::get("audio")->error("failed to initialize context");
        m_ok = false;
        return;
    }

    const auto backend_name = ma_get_backend_name(m_context.backend);
    spdlog::get("audio")->info("Audio backend: {}", backend_name);

    Enumerate();

    m_playback_config = ma_device_config_init(ma_device_type_playback);
    m_playback_config.playback.format = ma_format_f32;
    m_playback_config.playback.channels = 2;
    m_playback_config.sampleRate = 48000;
    m_playback_config.dataCallback = data_callback;
    m_playback_config.pUserData = this;

    if (const auto playback_id = m_devices.GetDefaultPlayback(); playback_id.has_value()) {
        m_playback_id = *playback_id;
        m_playback_config.playback.pDeviceID = &m_playback_id;

        if (auto code = ma_device_init(&m_context, &m_playback_config, &m_playback_device); code != MA_SUCCESS) {
            spdlog::get("audio")->error("failed to initialize playback device (code: {})", static_cast<int>(code));
            m_ok = false;
            return;
        }

        if (auto code = ma_device_start(&m_playback_device); code != MA_SUCCESS) {
            spdlog::get("audio")->error("failed to start playback (code: {})", static_cast<int>(code));
            ma_device_uninit(&m_playback_device);
            m_ok = false;
            return;
        }

        char playback_device_name[MA_MAX_DEVICE_NAME_LENGTH + 1];
        ma_device_get_name(&m_playback_device, ma_device_type_playback, playback_device_name, sizeof(playback_device_name), nullptr);
        spdlog::get("audio")->info("using {} as playback device", playback_device_name);
    }

    m_capture_config = ma_device_config_init(ma_device_type_capture);
    m_capture_config.capture.format = ma_format_s16;
    m_capture_config.capture.channels = 2;
    m_capture_config.sampleRate = 48000;
    m_capture_config.periodSizeInFrames = 480;
    m_capture_config.dataCallback = capture_data_callback;
    m_capture_config.pUserData = this;

    if (const auto capture_id = m_devices.GetDefaultCapture(); capture_id.has_value()) {
        m_capture_id = *capture_id;
        m_capture_config.capture.pDeviceID = &m_capture_id;

        if (auto code = ma_device_init(&m_context, &m_capture_config, &m_capture_device); code != MA_SUCCESS) {
            spdlog::get("audio")->error("failed to initialize capture device (code: {})", static_cast<int>(code));
            m_ok = false;
            return;
        }

        char capture_device_name[MA_MAX_DEVICE_NAME_LENGTH + 1];
        ma_device_get_name(&m_capture_device, ma_device_type_capture, capture_device_name, sizeof(capture_device_name), nullptr);
        spdlog::get("audio")->info("using {} as capture device", capture_device_name);
    }

    Glib::signal_timeout().connect(sigc::mem_fun(*this, &AudioManager::DecayVolumeMeters), 40);
}

AudioManager::~AudioManager() {
    ma_device_uninit(&m_playback_device);
    ma_device_uninit(&m_capture_device);
    ma_context_uninit(&m_context);
    RemoveAllSSRCs();

#ifdef WITH_RNNOISE
    RNNoiseUninitialize();
#endif
}

void AudioManager::AddSSRC(uint32_t ssrc) {
    std::lock_guard<std::mutex> _(m_mutex);
    int error;
    if (m_sources.find(ssrc) == m_sources.end()) {
        auto *decoder = opus_decoder_create(48000, 2, &error);
        m_sources.insert(std::make_pair(ssrc, std::make_pair(std::deque<int16_t> {}, decoder)));
    }
}

void AudioManager::RemoveSSRC(uint32_t ssrc) {
    std::lock_guard<std::mutex> _(m_mutex);
    if (auto it = m_sources.find(ssrc); it != m_sources.end()) {
        opus_decoder_destroy(it->second.second);
        m_sources.erase(it);
    }
}

void AudioManager::RemoveAllSSRCs() {
    spdlog::get("audio")->info("removing all ssrc");
    std::lock_guard<std::mutex> _(m_mutex);
    for (auto &[ssrc, pair] : m_sources) {
        opus_decoder_destroy(pair.second);
    }
    m_sources.clear();
}

void AudioManager::SetOpusBuffer(uint8_t *ptr) {
    m_opus_buffer = ptr;
}

void AudioManager::FeedMeOpus(uint32_t ssrc, const std::vector<uint8_t> &data) {
    if (!m_should_playback || ma_device_get_state(&m_playback_device) != ma_device_state_started) return;

    std::lock_guard<std::mutex> _(m_mutex);
    if (m_muted_ssrcs.find(ssrc) != m_muted_ssrcs.end()) return;

    size_t payload_size = 0;
    const auto *opus_encoded = StripRTPExtensionHeader(data.data(), static_cast<int>(data.size()), payload_size);
    static std::array<opus_int16, 120 * 48 * 2> pcm;
    if (auto it = m_sources.find(ssrc); it != m_sources.end()) {
        int decoded = opus_decode(it->second.second, opus_encoded, static_cast<opus_int32>(payload_size), pcm.data(), 120 * 48, 0);
        if (decoded <= 0) {
        } else {
            UpdateReceiveVolume(ssrc, pcm.data(), decoded);
            auto &buf = it->second.first;
            buf.insert(buf.end(), pcm.begin(), pcm.begin() + decoded * 2);
        }
    }
}

void AudioManager::StartCaptureDevice() {
    if (ma_device_start(&m_capture_device) != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to start capture device");
    }
}

void AudioManager::StopCaptureDevice() {
    if (ma_device_stop(&m_capture_device) != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to stop capture device");
    }
}

void AudioManager::SetPlaybackDevice(const Gtk::TreeModel::iterator &iter) {
    spdlog::get("audio")->debug("Setting new playback device");

    const auto device_id = m_devices.GetPlaybackDeviceIDFromModel(iter);
    if (!device_id) {
        spdlog::get("audio")->error("Requested ID from iterator is invalid");
        return;
    }

    m_devices.SetActivePlaybackDevice(iter);

    m_playback_id = *device_id;

    ma_device_uninit(&m_playback_device);

    m_playback_config = ma_device_config_init(ma_device_type_playback);
    m_playback_config.playback.format = ma_format_f32;
    m_playback_config.playback.channels = 2;
    m_playback_config.playback.pDeviceID = &m_playback_id;
    m_playback_config.sampleRate = 48000;
    m_playback_config.dataCallback = data_callback;
    m_playback_config.pUserData = this;

    if (ma_device_init(&m_context, &m_playback_config, &m_playback_device) != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to initialize new device");
        return;
    }

    if (ma_device_start(&m_playback_device) != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to start new device");
        return;
    }
}

void AudioManager::SetCaptureDevice(const Gtk::TreeModel::iterator &iter) {
    spdlog::get("audio")->debug("Setting new capture device");

    const auto device_id = m_devices.GetCaptureDeviceIDFromModel(iter);
    if (!device_id) {
        spdlog::get("audio")->error("Requested ID from iterator is invalid");
        return;
    }

    m_devices.SetActiveCaptureDevice(iter);

    m_capture_id = *device_id;

    ma_device_uninit(&m_capture_device);

    m_capture_config = ma_device_config_init(ma_device_type_capture);
    m_capture_config.capture.format = ma_format_s16;
    m_capture_config.capture.channels = 2;
    m_capture_config.capture.pDeviceID = &m_capture_id;
    m_capture_config.sampleRate = 48000;
    m_capture_config.periodSizeInFrames = 480;
    m_capture_config.dataCallback = capture_data_callback;
    m_capture_config.pUserData = this;

    if (ma_device_init(&m_context, &m_capture_config, &m_capture_device) != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to initialize new device");
        return;
    }

    // technically this should probably try and check old state but if you are in the window to change it then you are connected
    if (ma_device_start(&m_capture_device) != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to start new device");
        return;
    }
}

void AudioManager::SetCapture(bool capture) {
    m_should_capture = capture;
}

void AudioManager::SetPlayback(bool playback) {
    m_should_playback = playback;
}

void AudioManager::SetCaptureGate(double gate) {
    m_capture_gate = gate;
}

void AudioManager::SetCaptureGain(double gain) {
    m_capture_gain = gain;
}

double AudioManager::GetCaptureGate() const noexcept {
    return m_capture_gate;
}

double AudioManager::GetCaptureGain() const noexcept {
    return m_capture_gain;
}

void AudioManager::SetMuteSSRC(uint32_t ssrc, bool mute) {
    std::lock_guard<std::mutex> _(m_mutex);
    if (mute) {
        m_muted_ssrcs.insert(ssrc);
    } else {
        m_muted_ssrcs.erase(ssrc);
    }
}

void AudioManager::SetVolumeSSRC(uint32_t ssrc, double volume) {
    std::lock_guard<std::mutex> _(m_mutex);
    m_volume_ssrc[ssrc] = volume;
}

double AudioManager::GetVolumeSSRC(uint32_t ssrc) const {
    std::lock_guard<std::mutex> _(m_mutex);
    if (const auto iter = m_volume_ssrc.find(ssrc); iter != m_volume_ssrc.end()) {
        return iter->second;
    }
    return 1.0;
}

void AudioManager::SetEncodingApplication(int application) {
    std::lock_guard<std::mutex> _(m_enc_mutex);
    int prev_bitrate = 64000;
    if (int err = opus_encoder_ctl(m_encoder, OPUS_GET_BITRATE(&prev_bitrate)); err != OPUS_OK) {
        spdlog::get("audio")->error("Failed to get old bitrate when reinitializing: {}", err);
    }
    opus_encoder_destroy(m_encoder);
    int err = 0;
    m_encoder = opus_encoder_create(48000, 2, application, &err);
    if (err != OPUS_OK) {
        spdlog::get("audio")->critical("opus_encoder_create failed: {}", err);
        return;
    }

    if (int err = opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(prev_bitrate)); err != OPUS_OK) {
        spdlog::get("audio")->error("Failed to set bitrate when reinitializing: {}", err);
    }
}

int AudioManager::GetEncodingApplication() {
    std::lock_guard<std::mutex> _(m_enc_mutex);
    int temp = OPUS_APPLICATION_VOIP;
    if (int err = opus_encoder_ctl(m_encoder, OPUS_GET_APPLICATION(&temp)); err != OPUS_OK) {
        spdlog::get("audio")->error("opus_encoder_ctl(OPUS_GET_APPLICATION) failed: {}", err);
    }
    return temp;
}

void AudioManager::SetSignalHint(int signal) {
    std::lock_guard<std::mutex> _(m_enc_mutex);
    if (int err = opus_encoder_ctl(m_encoder, OPUS_SET_SIGNAL(signal)); err != OPUS_OK) {
        spdlog::get("audio")->error("opus_encoder_ctl(OPUS_SET_SIGNAL) failed: {}", err);
    }
}

int AudioManager::GetSignalHint() {
    std::lock_guard<std::mutex> _(m_enc_mutex);
    int temp = OPUS_AUTO;
    if (int err = opus_encoder_ctl(m_encoder, OPUS_GET_SIGNAL(&temp)); err != OPUS_OK) {
        spdlog::get("audio")->error("opus_encoder_ctl(OPUS_GET_SIGNAL) failed: {}", err);
    }
    return temp;
}

void AudioManager::SetBitrate(int bitrate) {
    std::lock_guard<std::mutex> _(m_enc_mutex);
    if (int err = opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(bitrate)); err != OPUS_OK) {
        spdlog::get("audio")->error("opus_encoder_ctl(OPUS_SET_BITRATE) failed: {}", err);
    }
}

int AudioManager::GetBitrate() {
    std::lock_guard<std::mutex> _(m_enc_mutex);
    int temp = 64000;
    if (int err = opus_encoder_ctl(m_encoder, OPUS_GET_BITRATE(&temp)); err != OPUS_OK) {
        spdlog::get("audio")->error("opus_encoder_ctl(OPUS_GET_BITRATE) failed: {}", err);
    }
    return temp;
}

void AudioManager::Enumerate() {
    ma_device_info *pPlaybackDeviceInfo;
    ma_uint32 playbackDeviceCount;
    ma_device_info *pCaptureDeviceInfo;
    ma_uint32 captureDeviceCount;

    spdlog::get("audio")->debug("Enumerating devices");

    if (ma_context_get_devices(
            &m_context,
            &pPlaybackDeviceInfo,
            &playbackDeviceCount,
            &pCaptureDeviceInfo,
            &captureDeviceCount) != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to enumerate devices");
        return;
    }

    spdlog::get("audio")->debug("Found {} playback devices and {} capture devices", playbackDeviceCount, captureDeviceCount);

    m_devices.SetDevices(pPlaybackDeviceInfo, playbackDeviceCount, pCaptureDeviceInfo, captureDeviceCount);
}

void AudioManager::OnCapturedPCM(const int16_t *pcm, ma_uint32 frames) {
    if (m_opus_buffer == nullptr || !m_should_capture) return;

    const double gain = m_capture_gain;

    std::vector<int16_t> new_pcm(pcm, pcm + frames * 2);
    for (auto &val : new_pcm) {
        const int32_t unclamped = static_cast<int32_t>(val * gain);
        val = std::clamp(unclamped, INT16_MIN, INT16_MAX);
    }

    if (m_mix_mono) {
        for (size_t i = 0; i < frames * 2; i += 2) {
            const int sample_L = new_pcm[i];
            const int sample_R = new_pcm[i + 1];
            const int16_t mixed = static_cast<int16_t>((sample_L + sample_R) / 2);
            new_pcm[i] = mixed;
            new_pcm[i + 1] = mixed;
        }
    }

    UpdateCaptureVolume(new_pcm.data(), frames);

    static std::array<float, 480> denoised_L;
    static std::array<float, 480> denoised_R;

    bool m_rnnoise_passed = false;
#ifdef WITH_RNNOISE
    if (m_vad_method == VADMethod::RNNoise || m_enable_noise_suppression) {
        m_rnnoise_passed = CheckVADRNNoise(new_pcm.data(), denoised_L.data(), denoised_R.data());
    }
#endif

    switch (m_vad_method) {
        case VADMethod::Gate:
            if (!CheckVADVoiceGate()) return;
            break;
#ifdef WITH_RNNOISE
        case VADMethod::RNNoise:
            if (!m_rnnoise_passed) return;
            break;
#endif
    }

    m_enc_mutex.lock();
    int payload_len = -1;

    if (m_enable_noise_suppression) {
        static std::array<int16_t, 960> denoised_interleaved;
        for (size_t i = 0; i < 480; i++) {
            denoised_interleaved[i * 2] = static_cast<int16_t>(denoised_L[i]);
        }
        for (size_t i = 0; i < 480; i++) {
            denoised_interleaved[i * 2 + 1] = static_cast<int16_t>(denoised_R[i]);
        }
        payload_len = opus_encode(m_encoder, denoised_interleaved.data(), 480, static_cast<unsigned char *>(m_opus_buffer), 1275);
    } else {
        payload_len = opus_encode(m_encoder, new_pcm.data(), 480, static_cast<unsigned char *>(m_opus_buffer), 1275);
    }

    m_enc_mutex.unlock();
    if (payload_len < 0) {
        spdlog::get("audio")->error("encoding error: {}", payload_len);
    } else {
        m_signal_opus_packet.emit(payload_len);
    }
}

void AudioManager::UpdateReceiveVolume(uint32_t ssrc, const int16_t *pcm, int frames) {
    std::lock_guard<std::mutex> _(m_vol_mtx);

    auto &meter = m_volumes[ssrc];
    for (int i = 0; i < frames * 2; i += 2) {
        const int amp = std::abs(pcm[i]);
        meter = std::max(meter, std::abs(amp) / 32768.0);
    }
}

void AudioManager::UpdateCaptureVolume(const int16_t *pcm, ma_uint32 frames) {
    for (ma_uint32 i = 0; i < frames * 2; i += 2) {
        const int amp = std::abs(pcm[i]);
        m_capture_peak_meter = std::max(m_capture_peak_meter.load(std::memory_order_relaxed), amp);
    }
}

bool AudioManager::DecayVolumeMeters() {
    m_capture_peak_meter -= 600;
    if (m_capture_peak_meter < 0) m_capture_peak_meter = 0;

    const auto x = m_vad_prob.load() - 0.05f;
    m_vad_prob.store(x < 0.0f ? 0.0f : x);

    std::lock_guard<std::mutex> _(m_vol_mtx);

    for (auto &[ssrc, meter] : m_volumes) {
        meter -= 0.01;
        if (meter < 0.0) meter = 0.0;
    }

    return true;
}

bool AudioManager::CheckVADVoiceGate() {
    return m_capture_peak_meter / 32768.0 > m_capture_gate;
}

#ifdef WITH_RNNOISE
bool AudioManager::CheckVADRNNoise(const int16_t *pcm, float *denoised_left, float *denoised_right) {
    // use left channel for vad, only denoise right if noise suppression enabled
    std::unique_lock<std::mutex> _(m_rnn_mutex);

    static float rnnoise_input[480];
    for (size_t i = 0; i < 480; i++) {
        rnnoise_input[i] = static_cast<float>(pcm[i * 2]);
    }
    m_vad_prob = std::max(m_vad_prob.load(), rnnoise_process_frame(m_rnnoise[0], denoised_left, rnnoise_input));

    if (m_enable_noise_suppression) {
        for (size_t i = 0; i < 480; i++) {
            rnnoise_input[i] = static_cast<float>(pcm[i * 2 + 1]);
        }
        rnnoise_process_frame(m_rnnoise[1], denoised_right, rnnoise_input);
    }

    return m_vad_prob > m_prob_threshold;
}

void AudioManager::RNNoiseInitialize() {
    spdlog::get("audio")->debug("Initializing RNNoise");
    RNNoiseUninitialize();
    std::unique_lock<std::mutex> _(m_rnn_mutex);
    m_rnnoise[0] = rnnoise_create(nullptr);
    m_rnnoise[1] = rnnoise_create(nullptr);
    const auto expected = rnnoise_get_frame_size();
    if (expected != 480) {
        spdlog::get("audio")->warn("RNNoise expects a frame count other than 480");
    }
}

void AudioManager::RNNoiseUninitialize() {
    if (m_rnnoise[0] != nullptr) {
        spdlog::get("audio")->debug("Uninitializing RNNoise");
        std::unique_lock<std::mutex> _(m_rnn_mutex);
        rnnoise_destroy(m_rnnoise[0]);
        rnnoise_destroy(m_rnnoise[1]);
        m_rnnoise[0] = nullptr;
        m_rnnoise[1] = nullptr;
    }
}
#endif

bool AudioManager::OK() const {
    return m_ok;
}

double AudioManager::GetCaptureVolumeLevel() const noexcept {
    return m_capture_peak_meter / 32768.0;
}

double AudioManager::GetSSRCVolumeLevel(uint32_t ssrc) const noexcept {
    std::lock_guard<std::mutex> _(m_vol_mtx);
    if (const auto it = m_volumes.find(ssrc); it != m_volumes.end()) {
        return it->second;
    }
    return 0.0;
}

AudioDevices &AudioManager::GetDevices() {
    return m_devices;
}

uint32_t AudioManager::GetRTPTimestamp() const noexcept {
    return m_rtp_timestamp;
}

void AudioManager::SetVADMethod(const std::string &method) {
    spdlog::get("audio")->debug("Setting VAD method to {}", method);
    if (method == "gate") {
        SetVADMethod(VADMethod::Gate);
    } else if (method == "rnnoise") {
#ifdef WITH_RNNOISE
        SetVADMethod(VADMethod::RNNoise);
#else
        SetVADMethod(VADMethod::Gate);
        spdlog::get("audio")->error("Tried to set RNNoise VAD method with support disabled");
#endif
    } else {
        SetVADMethod(VADMethod::Gate);
        spdlog::get("audio")->error("Tried to set unknown VAD method {}", method);
    }
}

void AudioManager::SetVADMethod(VADMethod method) {
    const auto method_int = static_cast<int>(method);
    spdlog::get("audio")->debug("Setting VAD method to enum {}", method_int);
    m_vad_method = method;
}

AudioManager::VADMethod AudioManager::GetVADMethod() const {
    return m_vad_method;
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
    return m_vad_prob;
}

double AudioManager::GetRNNProbThreshold() const {
    return m_prob_threshold;
}

void AudioManager::SetRNNProbThreshold(double value) {
    m_prob_threshold = value;
}

void AudioManager::SetSuppressNoise(bool value) {
    m_enable_noise_suppression = value;
}

bool AudioManager::GetSuppressNoise() const {
    return m_enable_noise_suppression;
}
#endif

void AudioManager::SetMixMono(bool value) {
    m_mix_mono = value;
}

bool AudioManager::GetMixMono() const {
    return m_mix_mono;
}

AudioManager::type_signal_opus_packet AudioManager::signal_opus_packet() {
    return m_signal_opus_packet;
}

#endif
