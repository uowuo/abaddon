#include "voice_capture.hpp"
#include "constants.hpp"

#include <spdlog/spdlog.h>

namespace AbaddonClient::Audio::Voice {

using OpusEncoder = Opus::OpusEncoder;
using EncoderSettings = OpusEncoder::EncoderSettings;
using SignalHint = OpusEncoder::SignalHint;
using EncodingApplication = OpusEncoder::EncodingApplication;

void capture_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    auto capture = static_cast<VoiceCapture*>(pDevice->pUserData);
    if (capture == nullptr) {
        return;
    }

    const auto buffer = InputBuffer(static_cast<const float*>(pInput), CAPTURE_BUFFER_SIZE);
    capture->OnAudioCapture(buffer);
}

VoiceCapture::VoiceCapture(Context &context) noexcept :
    m_device(context, GetDeviceConfig(), context.GetActiveCaptureID()) {}

void VoiceCapture::Start() noexcept {
    StartEncoder();
    if (!m_encoder.Lock()->has_value()) {
        return;
    }

    m_device.Start();
}

void VoiceCapture::Stop() noexcept {
    m_device.Stop();
    m_encoder.Lock()->reset();
}

void VoiceCapture::SetActive(bool active) noexcept {
    m_active = active;
}

void VoiceCapture::SetCaptureDevice(const ma_device_id &device_id) noexcept {
    spdlog::get("voice")->info("Setting capture device");

    const auto success = m_device.ChangeDevice(device_id);
    if (!success) {
        spdlog::get("voice")->error("Failed to set capture device");
    }
}

ma_device_config VoiceCapture::GetDeviceConfig() noexcept {
    auto config = ma_device_config_init(ma_device_type_capture);
    config.capture.format = CAPTURE_FORMAT;
    config.sampleRate = CAPTURE_SAMPLE_RATE;
    config.capture.channels = CAPTURE_CHANNELS;
    config.pulse.pStreamNameCapture = "Abaddon (Capture)";

    config.periodSizeInFrames = CAPTURE_FRAME_SIZE;
    config.performanceProfile = ma_performance_profile_low_latency;

    config.dataCallback = capture_callback;
    config.pUserData = this;

    return config;
}

void VoiceCapture::StartEncoder() noexcept {
    EncoderSettings settings;
    settings.sample_rate = CAPTURE_SAMPLE_RATE;
    settings.channels = CAPTURE_CHANNELS;
    settings.bitrate = 64000;
    settings.signal_hint = SignalHint::Auto;
    settings.application = EncodingApplication::VOIP;

    auto encoder = OpusEncoder::Create(settings);
    if (!encoder) {
        return;
    }

    m_encoder.Lock()->emplace(std::move(*encoder));
}

void VoiceCapture::OnAudioCapture(InputBuffer input) noexcept {
    if (!m_active) {
        return;
    }

    CaptureBuffer buffer;
    std::copy(input.begin(), input.end(), buffer.begin());

    ApplyEffects(buffer);
    if (ApplyNoise(buffer)) {
        EncodeAndSend(buffer);
    }
}

void VoiceCapture::ApplyEffects(CaptureBuffer &buffer) noexcept {
    if (MixMono) {
        AudioUtils::MixStereoToMono(buffer);
    }

    AudioUtils::ApplyGain(buffer, Gain);
    m_peak_meter.UpdatePeak(buffer);
}

bool VoiceCapture::ApplyNoise(CaptureBuffer &buffer) noexcept {
    if(!m_effects.PassesVAD(buffer, m_peak_meter.GetPeak())) {
        return false;
    }

    if (SuppressNoise) {
        m_effects.Denoise(buffer);
    }

    return true;
}

void VoiceCapture::EncodeAndSend(const CaptureBuffer &buffer) noexcept {
    std::vector<uint8_t> opus;
    opus.resize(1275);

    const auto bytes = m_encoder.Lock()->value().Encode(buffer, opus, CAPTURE_FRAME_SIZE);
    opus.resize(bytes);

    if (bytes > 0) {
        m_signal.emit(opus);
    }

    m_rtp_timestamp += CAPTURE_FRAME_SIZE;
}

Capture::VoiceEffects& VoiceCapture::GetEffects() noexcept {
    return m_effects;
}

const Capture::VoiceEffects& VoiceCapture::GetEffects() const noexcept {
    return m_effects;
}

PeakMeter& VoiceCapture::GetPeakMeter() noexcept {
    return m_peak_meter;
}

const PeakMeter& VoiceCapture::GetPeakMeter() const noexcept {
    return m_peak_meter;
}

MutexGuard<std::optional<OpusEncoder>> VoiceCapture::GetEncoder() noexcept {
    return m_encoder.Lock();
}

const MutexGuard<std::optional<OpusEncoder>> VoiceCapture::GetEncoder() const noexcept {
    return m_encoder.Lock();
}

VoiceCapture::CaptureSignal VoiceCapture::GetCaptureSignal() const noexcept {
    return m_signal;
}

uint32_t VoiceCapture::GetRTPTimestamp() const noexcept {
    return m_rtp_timestamp;
}


}
