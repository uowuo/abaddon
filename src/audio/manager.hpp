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
#include <sigc++/sigc++.h>
// clang-format on

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void AddSSRC(uint32_t ssrc);
    void RemoveSSRC(uint32_t ssrc);
    void RemoveAllSSRCs();

    void SetOpusBuffer(uint8_t *ptr);
    void FeedMeOpus(uint32_t ssrc, const std::vector<uint8_t> &data);

    void SetCapture(bool capture);
    void SetPlayback(bool playback);

    [[nodiscard]] bool OK() const;

private:
    void OnCapturedPCM(const int16_t *pcm, ma_uint32 frames);

    friend void data_callback(ma_device *, void *, const void *, ma_uint32);
    friend void capture_data_callback(ma_device *, void *, const void *, ma_uint32);

    std::thread m_thread;

    bool m_ok;

    // playback
    ma_device m_device;
    ma_device_config m_device_config;
    // capture
    ma_device m_capture_device;
    ma_device_config m_capture_config;

    std::mutex m_mutex;
    std::unordered_map<uint32_t, std::pair<std::deque<int16_t>, OpusDecoder *>> m_sources;

    OpusEncoder *m_encoder;

    uint8_t *m_opus_buffer = nullptr;

    std::atomic<bool> m_should_capture = true;
    std::atomic<bool> m_should_playback = true;

public:
    using type_signal_opus_packet = sigc::signal<void(int payload_size)>;
    type_signal_opus_packet signal_opus_packet();

private:
    type_signal_opus_packet m_signal_opus_packet;
};
#endif
