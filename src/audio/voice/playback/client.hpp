#pragma once

#include "audio/voice/opus/opus_decoder.hpp"
#include "audio/voice/peak_meter/peak_meter.hpp"

#include "decode_pool.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

class Client {
public:
    Client(Opus::OpusDecoder &&decoder, VoiceBuffer &&buffer, DecodePool &decode_pool) noexcept;

    void DecodeFromRTP(const std::vector<u_int8_t> &&rtp) noexcept;
    void WriteAudio(OutputBuffer output) noexcept;

    void ClearBuffer() noexcept;

    void SetMuted(bool muted) noexcept;
    bool GetMuted() const noexcept;

    PeakMeter& GetPeakMeter() noexcept;
    const PeakMeter& GetPeakMeter() const noexcept;

    std::atomic<float> m_volume = 1.0;

private:
    SharedDecoder m_decoder;
    SharedBuffer m_buffer;

    DecodePool &m_decode_pool;
    PeakMeter m_peak_meter;

    std::atomic<bool> m_muted = false;
};

}
