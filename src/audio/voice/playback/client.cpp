#include "client.hpp"
#include "constants.hpp"

#include "abaddon.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

void client_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const auto playback_active = Abaddon::Get().GetAudio().GetVoice().GetPlayback().GetActive();
    if (!playback_active) {
        return;
    }

    auto client = static_cast<Client*>(pDevice->pUserData);
    auto buffer = OutputBuffer(static_cast<float*>(pOutput), frameCount * RTP_CHANNELS);

    client->WriteAudio(buffer);
    AudioUtils::ClampToFloatRange(buffer);
}

Client::Client(Context &context, Opus::OpusDecoder &&decoder, VoiceBuffer &&buffer, DecodePool &decode_pool) noexcept :
    m_device(context, GetDeviceConfig(), context.GetActivePlaybackID()),
    m_decoder( std::make_shared< Mutex<Opus::OpusDecoder> >(std::move(decoder)) ),
    m_buffer( std::make_shared<VoiceBuffer>( std::move(buffer) )),
    m_decode_pool(decode_pool)
{
    if (Abaddon::Get().GetSettings().SeparateSources) {
        m_device.Start();
    }
}

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

ma_device_config Client::GetDeviceConfig() noexcept {
    auto config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = RTP_FORMAT;
    config.sampleRate = RTP_SAMPLE_RATE;
    config.playback.channels = RTP_CHANNELS;
    config.pulse.pStreamNamePlayback = "Abaddon (User)";

    config.noClip = true;
    config.noFixedSizedCallback = true;
    config.performanceProfile = ma_performance_profile_low_latency;

    config.dataCallback = client_callback;
    config.pUserData = this;

    return config;
}

}
