#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include <opus.h>

#include "audio/utils.hpp"

namespace AbaddonClient::Audio::Voice::Opus {

using OpusInput = ConstSlice<uint8_t>;

class OpusDecoder {
public:
    struct DecoderSettings {
        int32_t sample_rate = 48000;
        int channels = 2;
    };

    static std::optional<OpusDecoder> Create(DecoderSettings settings) noexcept;

    int Decode(OpusInput opus, OutputBuffer pcm, int frame_size) noexcept;
private:
    struct DecoderDeleter {
        void operator()(::OpusDecoder* ptr) noexcept {
            opus_decoder_destroy(ptr);
        }
    };

    using DecoderPtr = std::unique_ptr<::OpusDecoder, DecoderDeleter>;
    OpusDecoder(DecoderPtr encoder, DecoderSettings settings) noexcept;

    DecoderPtr m_encoder;
};

}
