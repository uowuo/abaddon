#pragma once

#include <opus.h>

#include "audio/utils.hpp"

namespace AbaddonClient::Audio::Voice::Opus {

using OpusInput = ConstSlice<uint8_t>;

class OpusDecoder {
public:
    using DecoderPtr = std::unique_ptr<::OpusDecoder, decltype(&opus_decoder_destroy)>;

    struct DecoderSettings {
        int32_t sample_rate = 48000;
        int channels = 2;
    };

    OpusDecoder(DecoderPtr encoder, DecoderSettings settings) noexcept;
    static std::optional<OpusDecoder> Create(DecoderSettings settings) noexcept;

    int Decode(OpusInput opus, OutputBuffer pcm, int frame_size) noexcept;
private:

    DecoderPtr m_encoder;
};

}
