#pragma once

#include "audio/audio_device.hpp"
#include "audio/context.hpp"
#include "audio/utils.hpp"

#include "audio/voice/opus/opus_encoder.hpp"
#include "audio/voice/peak_meter/peak_meter.hpp"

#include "voice_effects.hpp"

namespace AbaddonClient::Audio::Voice {

class VoiceCapture {
public:
    using CaptureSignal = sigc::signal<void (const std::vector<uint8_t>&)>;

    VoiceCapture(Context &context) noexcept;

    void Start() noexcept;
    void Stop() noexcept;

    void SetActive(bool active) noexcept;
    void SetCaptureDevice(const ma_device_id &device_id) noexcept;

    Capture::VoiceEffects& GetEffects() noexcept;
    const Capture::VoiceEffects& GetEffects() const noexcept;

    MutexGuard<std::optional<Opus::OpusEncoder>> GetEncoder() noexcept;
    const MutexGuard<std::optional<Opus::OpusEncoder>> GetEncoder() const noexcept;

    PeakMeter& GetPeakMeter() noexcept;
    const PeakMeter& GetPeakMeter() const noexcept;

    CaptureSignal GetCaptureSignal() const noexcept;
    uint32_t GetRTPTimestamp() const noexcept;

    std::atomic<float> Gain = 1.0f;
    std::atomic<bool> MixMono = false;
    std::atomic<bool> SuppressNoise = false;
private:
    ma_device_config GetDeviceConfig() noexcept;
    void StartEncoder() noexcept;

    void OnAudioCapture(const InputBuffer input_buffer) noexcept;

    void ApplyEffects(CaptureBuffer &buffer) noexcept;
    bool ApplyNoise(CaptureBuffer &buffer) noexcept;
    void EncodeAndSend(const CaptureBuffer &buffer) noexcept;

    Capture::VoiceEffects m_effects;
    PeakMeter m_peak_meter;

    AudioDevice m_device;
    Mutex<std::optional<Opus::OpusEncoder>> m_encoder;
    CaptureSignal m_signal;

    // TODO: Ideally this should not be here
    // RTP should be handled by VoiceClient
    std::atomic<uint32_t> m_rtp_timestamp = 0;
    std::atomic<bool> m_active = true;

    friend void capture_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};

}
