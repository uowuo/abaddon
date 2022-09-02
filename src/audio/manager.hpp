#pragma once
#ifdef WITH_VOICE
// clang-format off
#include <array>
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <miniaudio.h>
#include <opus.h>
// clang-format on

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void FeedMeOpus(uint32_t ssrc, const std::vector<uint8_t> &data);

    [[nodiscard]] bool OK() const;

private:
    friend void data_callback(ma_device *, void *, const void *, ma_uint32);

    std::thread m_thread;

    bool m_ok;

    ma_device m_device;
    ma_device_config m_device_config;

    std::mutex m_mutex;
    std::unordered_map<uint32_t, std::pair<std::deque<int16_t>, OpusDecoder *>> m_sources;
};
#endif
