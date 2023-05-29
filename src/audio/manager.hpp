#pragma once
#ifdef WITH_VOICE
// clang-format off

#include <array>
#include <atomic>
#include <deque>
#include <gtkmm/treemodel.h>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <miniaudio.h>
#include <opus.h>
#include <sigc++/sigc++.h>
#include "devices.hpp"
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

    void StartCaptureDevice();
    void StopCaptureDevice();

    void SetPlaybackDevice(const Gtk::TreeModel::iterator &iter);
    void SetCaptureDevice(const Gtk::TreeModel::iterator &iter);

    void SetCapture(bool capture);
    void SetPlayback(bool playback);

    void SetCaptureGate(double gate);
    void SetCaptureGain(double gain);
    [[nodiscard]] double GetCaptureGate() const noexcept;
    [[nodiscard]] double GetCaptureGain() const noexcept;

    void SetMuteSSRC(uint32_t ssrc, bool mute);
    void SetVolumeSSRC(uint32_t ssrc, double volume);
    [[nodiscard]] double GetVolumeSSRC(uint32_t ssrc) const;

    void SetEncodingApplication(int application);
    [[nodiscard]] int GetEncodingApplication();
    void SetSignalHint(int signal);
    [[nodiscard]] int GetSignalHint();
    void SetBitrate(int bitrate);
    [[nodiscard]] int GetBitrate();

    void Enumerate();

    [[nodiscard]] bool OK() const;

    [[nodiscard]] double GetCaptureVolumeLevel() const noexcept;
    [[nodiscard]] double GetSSRCVolumeLevel(uint32_t ssrc) const noexcept;

    [[nodiscard]] AudioDevices &GetDevices();

    [[nodiscard]] uint32_t GetRTPTimestamp() const noexcept;

private:
    void OnCapturedPCM(const int16_t *pcm, ma_uint32 frames);

    void UpdateReceiveVolume(uint32_t ssrc, const int16_t *pcm, int frames);
    void UpdateCaptureVolume(const int16_t *pcm, ma_uint32 frames);
    std::atomic<int> m_capture_peak_meter = 0;

    bool DecayVolumeMeters();

    friend void data_callback(ma_device *, void *, const void *, ma_uint32);
    friend void capture_data_callback(ma_device *, void *, const void *, ma_uint32);

    std::thread m_thread;

    bool m_ok;

    // playback
    ma_device m_playback_device;
    ma_device_config m_playback_config;
    ma_device_id m_playback_id;
    // capture
    ma_device m_capture_device;
    ma_device_config m_capture_config;
    ma_device_id m_capture_id;

    ma_context m_context;

    mutable std::mutex m_mutex;
    mutable std::mutex m_enc_mutex;

    std::unordered_map<uint32_t, std::pair<std::deque<int16_t>, OpusDecoder *>> m_sources;

    OpusEncoder *m_encoder;

    uint8_t *m_opus_buffer = nullptr;

    std::atomic<bool> m_should_capture = true;
    std::atomic<bool> m_should_playback = true;

    std::atomic<double> m_capture_gate = 0.0;
    std::atomic<double> m_capture_gain = 1.0;

    std::unordered_set<uint32_t> m_muted_ssrcs;
    std::unordered_map<uint32_t, double> m_volume_ssrc;

    mutable std::mutex m_vol_mtx;
    std::unordered_map<uint32_t, double> m_volumes;

    AudioDevices m_devices;

    std::atomic<uint32_t> m_rtp_timestamp = 0;

public:
    using type_signal_opus_packet = sigc::signal<void(int payload_size)>;
    type_signal_opus_packet signal_opus_packet();

private:
    type_signal_opus_packet m_signal_opus_packet;
};
#endif
