#ifdef WITH_VOICE
    // clang-format off
#ifdef _WIN32
    #include <winsock2.h>
#endif

#include "manager.hpp"
#include <array>
#define MINIAUDIO_IMPLEMENTATION
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
        auto &buf = pair.first;
        const size_t n = std::min(static_cast<size_t>(buf.size()), static_cast<size_t>(frameCount * 2ULL));
        for (size_t i = 0; i < n; i++) {
            pOutputF32[i] += buf[i] / 32768.F;
        }
        buf.erase(buf.begin(), buf.begin() + n);
    }
}

void capture_data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    auto *mgr = reinterpret_cast<AudioManager *>(pDevice->pUserData);
    if (mgr == nullptr) return;

    mgr->OnCapturedPCM(static_cast<const int16_t *>(pInput), frameCount);
}

AudioManager::AudioManager() {
    m_ok = true;

    int err;
    m_encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK) {
        printf("failed to initialize opus encoder: %d\n", err);
        m_ok = false;
        return;
    }
    opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(64000));

    m_device_config = ma_device_config_init(ma_device_type_playback);
    m_device_config.playback.format = ma_format_f32;
    m_device_config.playback.channels = 2;
    m_device_config.sampleRate = 48000;
    m_device_config.dataCallback = data_callback;
    m_device_config.pUserData = this;

    if (ma_device_init(nullptr, &m_device_config, &m_device) != MA_SUCCESS) {
        puts("open playback fail");
        m_ok = false;
        return;
    }

    if (ma_device_start(&m_device) != MA_SUCCESS) {
        puts("failed to start playback");
        ma_device_uninit(&m_device);
        m_ok = false;
        return;
    }

    m_capture_config = ma_device_config_init(ma_device_type_capture);
    m_capture_config.capture.format = ma_format_s16;
    m_capture_config.capture.channels = 2;
    m_capture_config.sampleRate = 48000;
    m_capture_config.periodSizeInFrames = 480;
    m_capture_config.dataCallback = capture_data_callback;
    m_capture_config.pUserData = this;

    if (ma_device_init(nullptr, &m_capture_config, &m_capture_device) != MA_SUCCESS) {
        puts("open capture fail");
        m_ok = false;
        return;
    }

    if (ma_device_start(&m_capture_device) != MA_SUCCESS) {
        puts("failed to start capture");
        ma_device_uninit(&m_capture_device);
        m_ok = false;
        return;
    }

    char device_name[MA_MAX_DEVICE_NAME_LENGTH + 1];
    ma_device_get_name(&m_capture_device, ma_device_type_capture, device_name, sizeof(device_name), nullptr);
    printf("using %s for capture\n", device_name);
}

AudioManager::~AudioManager() {
    ma_device_uninit(&m_device);
    ma_device_uninit(&m_capture_device);
    for (auto &[ssrc, pair] : m_sources) {
        opus_decoder_destroy(pair.second);
    }
}

void AudioManager::SetOpusBuffer(uint8_t *ptr) {
    m_opus_buffer = ptr;
}

void AudioManager::FeedMeOpus(uint32_t ssrc, const std::vector<uint8_t> &data) {
    size_t payload_size = 0;
    const auto *opus_encoded = StripRTPExtensionHeader(data.data(), static_cast<int>(data.size()), payload_size);
    static std::array<opus_int16, 120 * 48 * 2> pcm;
    if (m_sources.find(ssrc) == m_sources.end()) {
        int err;
        auto *decoder = opus_decoder_create(48000, 2, &err);
        m_sources.insert(std::make_pair(ssrc, std::make_pair(std::deque<int16_t> {}, decoder)));
    }
    int decoded = opus_decode(m_sources.at(ssrc).second, opus_encoded, static_cast<opus_int32>(payload_size), pcm.data(), 120 * 48, 0);
    if (decoded <= 0) {
    } else {
        m_mutex.lock();
        auto &buf = m_sources.at(ssrc).first;
        buf.insert(buf.end(), pcm.begin(), pcm.begin() + decoded * 2);
        m_mutex.unlock();
    }
}

void AudioManager::OnCapturedPCM(const int16_t *pcm, ma_uint32 frames) {
    if (m_opus_buffer == nullptr) return;

    int payload_len = opus_encode(m_encoder, pcm, 480, static_cast<unsigned char *>(m_opus_buffer), 1275);
    if (payload_len < 0) {
        printf("encoding error: %d\n", payload_len);
    } else {
        m_signal_opus_packet.emit(payload_len);
    }
}

bool AudioManager::OK() const {
    return m_ok;
}

AudioManager::type_signal_opus_packet AudioManager::signal_opus_packet() {
    return m_signal_opus_packet;
}
#endif
