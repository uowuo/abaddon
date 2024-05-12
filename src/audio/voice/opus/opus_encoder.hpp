#pragma once

#include <opus.h>

#include "audio/utils.hpp"

namespace AbaddonClient::Audio::Voice::Opus {

using OpusOutput = Slice<uint8_t>;

class OpusEncoder {
public:
    using EncoderPtr = std::unique_ptr<::OpusEncoder, decltype(&opus_encoder_destroy)>;

    enum class EncodingApplication {
        Audio = OPUS_APPLICATION_AUDIO,
        LowDelay = OPUS_APPLICATION_RESTRICTED_LOWDELAY,
        VOIP = OPUS_APPLICATION_VOIP
    };

    enum class SignalHint {
        Auto = OPUS_AUTO,
        Voice = OPUS_SIGNAL_VOICE,
        Music = OPUS_SIGNAL_MUSIC
    };

    struct EncoderSettings {
        int32_t sample_rate = 48000;
        int channels = 2;
        int32_t bitrate = 64000;
        SignalHint signal_hint = SignalHint::Auto;
        EncodingApplication application = EncodingApplication::Audio;
    };

    OpusEncoder(EncoderPtr encoder, EncoderSettings settings) noexcept;
    static std::optional<OpusEncoder> Create(EncoderSettings settings) noexcept;

    int Encode(InputBuffer &&audio, OpusOutput &&opus, int frame_size) noexcept;
    void ResetState() noexcept;

    void SetBitrate(int32_t bitrate) noexcept;
    void SetSignalHint(SignalHint hint) noexcept;
    void SetEncodingApplication(EncodingApplication application) noexcept;

    int32_t GetBitrate() const noexcept;
    SignalHint GetSignalHint() const noexcept;
    EncodingApplication GetEncodingApplication() const noexcept;
private:
    template<typename... Args>
    bool OpusCTL(std::string_view request, Args&& ...args) noexcept;

    EncoderPtr m_encoder;

    const int32_t m_sample_rate;
    const int m_channels;

    int32_t m_bitrate;
    SignalHint m_signal_hint;
    EncodingApplication m_application;
};

}
