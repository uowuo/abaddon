#include "opus_decoder.hpp"

namespace AbaddonClient::Audio::Voice::Opus {

OpusDecoder::OpusDecoder(DecoderPtr encoder, DecoderSettings settings) noexcept :
    m_encoder(std::move(encoder)) {}

std::optional<OpusDecoder>
OpusDecoder::Create(const DecoderSettings settings) noexcept {
    int error;
    const auto decoder = opus_decoder_create(settings.sample_rate, settings.channels, &error);

    if (error != OPUS_OK) {
        spdlog::get("voice")->error("Failed to create opus decoder: {}", opus_strerror(error));
        return std::nullopt;
    }

    auto decoder_ptr = DecoderPtr(decoder);
    return OpusDecoder(std::move(decoder_ptr), settings);
}

int OpusDecoder::Decode(OpusInput opus, OutputBuffer output, const int frame_size) noexcept {
    const auto frames = opus_decode_float(m_encoder.get(), opus.data(), opus.size(), output.data(), frame_size, 0);
    if (frames < 0) {
        spdlog::get("voice")->error("Opus decoder error: {}", opus_strerror(frames));
    }

    return frames;
}

}
