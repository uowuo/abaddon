#include "opus_encoder.hpp"

namespace AbaddonClient::Audio::Voice::Opus {

using EncoderSettings = OpusEncoder::EncoderSettings;
using EncodingApplication = OpusEncoder::EncodingApplication;
using SignalHint = OpusEncoder::SignalHint;

OpusEncoder::OpusEncoder(EncoderPtr encoder, EncoderSettings settings) noexcept :
    m_encoder(std::move(encoder)),
    m_sample_rate(settings.sample_rate),
    m_channels(settings.channels),
    m_bitrate(settings.bitrate),
    m_signal_hint(settings.signal_hint),
    m_application(settings.application)
{
    SetBitrate(m_bitrate);
    SetSignalHint(m_signal_hint);
}

std::optional<OpusEncoder>
OpusEncoder::Create(EncoderSettings settings) noexcept {
    int error;
    const auto encoder = opus_encoder_create(settings.sample_rate, settings.channels, static_cast<int>(settings.application), &error);

    if (error != OPUS_OK) {
        spdlog::get("voice")->error("Cannot create opus encoder: {}", opus_strerror(error));
        return std::nullopt;
    }

    auto encoder_ptr = EncoderPtr(encoder);
    return OpusEncoder(std::move(encoder_ptr), settings);
}

int OpusEncoder::Encode(InputBuffer &&input, OpusOutput &&output, int frame_size) noexcept {
    const auto bytes = opus_encode_float(m_encoder.get(), input.data(), frame_size, output.data(), output.size());
    if (bytes < 0) {
        spdlog::get("voice")->error("Opus encoder error: {}", opus_strerror(bytes));
    }

    return bytes;
}

void OpusEncoder::ResetState() noexcept {
    OpusCTL("OPUS_RESET_STATE", OPUS_RESET_STATE);
}

void OpusEncoder::SetBitrate(int32_t bitrate) noexcept {
    const auto success = OpusCTL("OPUS_SET_BITRATE", OPUS_SET_BITRATE(bitrate));

    if (success) {
        m_bitrate = bitrate;
    }
}

void OpusEncoder::SetSignalHint(SignalHint hint) noexcept {
    const auto hint_int = static_cast<int>(hint);
    const auto success = OpusCTL("OPUS_SET_SIGNAL", OPUS_SET_SIGNAL(hint_int));

    if (success) {
        m_signal_hint = hint;
    }
}

void OpusEncoder::SetEncodingApplication(EncodingApplication application) noexcept {
    int error;
    const auto encoder = opus_encoder_create(m_sample_rate, m_channels, static_cast<int>(application), &error);

    if (error != OPUS_OK) {
        spdlog::get("voice")->error("Cannot change encoding application: {}", opus_strerror(error));
        return;
    }

    m_encoder.reset(encoder);

    SetBitrate(m_bitrate);
    SetSignalHint(m_signal_hint);
}

int32_t OpusEncoder::GetBitrate() const noexcept {
    return m_bitrate;
}

EncodingApplication OpusEncoder::GetEncodingApplication() const noexcept {
    return m_application;
}

SignalHint OpusEncoder::GetSignalHint() const noexcept {
    return m_signal_hint;
}

template<typename... Args>
bool OpusEncoder::OpusCTL(std::string_view request, Args&& ...args) noexcept {
    auto error = opus_encoder_ctl(m_encoder.get(), std::forward<Args>(args)...);
    if (error != OPUS_OK) {
        spdlog::get("voice")->error("Opus encoder CTL error ({}): {}", request, opus_strerror(error));
        return false;
    }

    return true;
}

}
