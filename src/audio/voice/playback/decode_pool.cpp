#include "decode_pool.hpp"
#include "constants.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

// I have no idea what this does, I just copied it from AudioManager
Opus::OpusInput StripRTPExtensionHeader(ConstSlice<uint8_t> rtp) {
    if (rtp[0] == 0xbe && rtp[1] == 0xde && rtp.size() > 4) {
        uint64_t offset = 4 + 4 * ((rtp[2] << 8) | rtp[3]);

        return Opus::OpusInput(rtp.data() + offset, rtp.size() - offset);
    }
    return rtp;
}

DecodePool::DecodePool() noexcept :
    m_pool(Pool(&DecodePool::DecodeThread, 20))
{
    auto concurrency = std::thread::hardware_concurrency() / 2;
    if (concurrency == 0) {
        concurrency = 2;
    }

    m_pool.m_max_threads = concurrency;
}

void DecodePool::DecodeFromRTP(DecodeData &&decode_data) noexcept {
    m_pool.SendToPool(std::move(decode_data));
}

void DecodePool::AddDecoder() noexcept {
    m_pool.AddThread();
}

void DecodePool::RemoveDecoder() noexcept {
    m_pool.RemoveThread();
}

void DecodePool::ClearDecoders() noexcept {
    m_pool.Clear();
}

size_t DecodePool::GetDecoderCount() const noexcept {
    return m_pool.GetThreadCount();
}

void DecodePool::DecodeThread(Channel<Pool::ThreadMessage> &channel) noexcept {
    while (true) {
        auto message = channel.Recv();
        if (std::holds_alternative<Terminate>(message)) {
            break;
        }

        auto&& decode_message = std::get<DecodeData>(message);
        OnDecodeMessage(std::move(decode_message));
    }
}

void DecodePool::OnDecodeMessage(DecodeData &&message) noexcept {
    auto&& [rtp, decoder, buffer] = message;

    static std::array<float, RTP_OPUS_MAX_BUFFER_SIZE> pcm;
    const auto opus = StripRTPExtensionHeader(rtp);

    int frames = decoder->Lock()->Decode(opus, pcm, RTP_OPUS_FRAME_SIZE);
    if (frames < 0) {
        return;
    }

    // Take only what we've got from the decoder
    auto pcm_written = OutputBuffer(pcm.data(), frames * RTP_CHANNELS);
    buffer->Write(pcm_written);
}

}
