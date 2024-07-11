#pragma once

#include "audio/voice/opus/opus_decoder.hpp"
#include "audio/voice/peak_meter/peak_meter.hpp"

#include "audio/audio_device.hpp"
#include "audio/context.hpp"

#include "decode_pool.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

class Client {
public:
    Client(Context &context, Opus::OpusDecoder &&decoder, VoiceBuffer &&buffer, DecodePool &decode_pool) noexcept;

    void Decode(std::vector<uint8_t> &&rtp) noexcept;
    void WriteAudio(OutputBuffer output) noexcept;

    void ClearBuffer() noexcept;

    void SetMuted(bool muted) noexcept;
    bool GetMuted() const noexcept;

    PeakMeter& GetPeakMeter() noexcept;
    const PeakMeter& GetPeakMeter() const noexcept;

    std::atomic<float> Volume = 1.0;

private:
    ma_device_config GetDeviceConfig() noexcept;

    AudioDevice m_device;

    SharedDecoder m_decoder;
    SharedBuffer m_buffer;

    DecodePool &m_decode_pool;
    PeakMeter m_peak_meter;

    std::atomic<bool> m_muted = false;

    friend void client_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};

}
