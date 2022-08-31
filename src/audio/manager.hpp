#pragma once
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <miniaudio.h>
#include <opus/opus.h>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void FeedMeOpus(const std::vector<uint8_t> &data);

    [[nodiscard]] bool OK() const;

private:
    friend void data_callback(ma_device *, void *, const void *, ma_uint32);

    std::atomic<bool> m_active;
    void testthread();
    std::thread m_thread;

    bool m_ok;

    ma_engine m_engine;
    ma_device m_device;
    ma_device_config m_device_config;

    std::mutex m_dumb_mutex;
    std::queue<int16_t> m_dumb;

    OpusDecoder *m_opus_decoder;
};
