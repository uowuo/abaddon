#ifdef _WIN32
    #include <winsock2.h>
#endif

#include "manager.hpp"
#include <array>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <opus/opus.h>
#include <cstring>

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
    std::lock_guard<std::mutex> _(mgr->m_dumb_mutex);

    const auto buffered_frames = std::min(static_cast<ma_uint32>(mgr->m_dumb.size() / 2), frameCount);
    auto *pOutputCast = static_cast<ma_int16 *>(pOutput);
    for (ma_uint32 i = 0; i < buffered_frames * 2; i++) {
        pOutputCast[i] = mgr->m_dumb.front();
        mgr->m_dumb.pop();
    }
}

AudioManager::AudioManager() {
    m_ok = true;

    m_device_config = ma_device_config_init(ma_device_type_playback);
    m_device_config.playback.format = ma_format_s16;
    m_device_config.playback.channels = 2;
    m_device_config.sampleRate = 48000;
    m_device_config.dataCallback = data_callback;
    m_device_config.pUserData = this;

    if (ma_device_init(nullptr, &m_device_config, &m_device) != MA_SUCCESS) {
        puts("open playabck fail");
        m_ok = false;
        return;
    }

    if (ma_device_start(&m_device) != MA_SUCCESS) {
        puts("failed to start playback");
        ma_device_uninit(&m_device);
        m_ok = false;
        return;
    }

    int err;
    m_opus_decoder = opus_decoder_create(48000, 2, &err);

    m_active = true;
    // m_thread = std::thread(&AudioManager::testthread, this);
}

AudioManager::~AudioManager() {
    m_active = false;
    ma_device_uninit(&m_device);
}

void AudioManager::FeedMeOpus(const std::vector<uint8_t> &data) {
    size_t payload_size = 0;
    const auto *opus_encoded = StripRTPExtensionHeader(data.data(), static_cast<int>(data.size()), payload_size);
    static std::array<opus_int16, 120 * 48 * 2 * sizeof(opus_int16)> pcm;
    int decoded = opus_decode(m_opus_decoder, opus_encoded, static_cast<opus_int32>(payload_size), pcm.data(), 120 * 48, 0);
    if (decoded <= 0) {
        printf("failed decode: %d\n", decoded);
    } else {
        m_dumb_mutex.lock();
        for (size_t i = 0; i < decoded * 2; i++) {
            m_dumb.push(pcm[i]);
        }
        m_dumb_mutex.unlock();
    }
}

void AudioManager::testthread() {
}

bool AudioManager::OK() const {
    return m_ok;
}
