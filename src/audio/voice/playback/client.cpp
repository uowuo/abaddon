#include "client.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

Client::Client(Opus::OpusDecoder &&decoder, VoiceBuffer &&buffer, DecodePool &decode_pool) noexcept :
    m_decoder( std::make_shared< Mutex<Opus::OpusDecoder> >(std::move(decoder)) ),
    m_buffer( std::make_shared<VoiceBuffer>( std::move(buffer) )),
    m_decode_pool(decode_pool) {}

void Client::DecodeFromRTP(std::vector<uint8_t> &&rtp) noexcept {
    if (m_muted) {
        return;
    }

    auto decode_data = DecodePool::DecodeData {
        std::move(rtp),
        m_decoder,
        m_buffer,
    };

    m_decode_pool.DecodeFromRTP(std::move(decode_data));
}

void Client::WriteAudio(OutputBuffer buffer) noexcept {
    if (m_muted) {
        return;
    }

    m_buffer->Read(buffer);
    AudioUtils::ApplyGain(buffer, Volume);

    m_peak_meter.UpdatePeak(buffer);
}

void Client::ClearBuffer() noexcept {
    m_buffer->Clear();
}

void Client::SetMuted(bool muted) noexcept {
    if (muted) {
        // Clear the buffer to prevent residue samples playing back later
        ClearBuffer();
    }

    m_muted = muted;
}

bool Client::GetMuted() const noexcept {
    return m_muted;
}

PeakMeter& Client::GetPeakMeter() noexcept {
    return m_peak_meter;
}

const PeakMeter& Client::GetPeakMeter() const noexcept {
    return m_peak_meter;
}

}
