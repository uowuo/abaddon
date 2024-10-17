#pragma once

#include <miniaudio.h>

constexpr ma_format RTP_FORMAT = ma_format_f32;
constexpr uint32_t RTP_SAMPLE_RATE = 48000;
constexpr uint32_t RTP_CHANNELS = 2;
constexpr uint32_t RTP_OPUS_FRAME_SIZE = 5760;
constexpr uint32_t RTP_OPUS_MAX_BUFFER_SIZE = RTP_OPUS_FRAME_SIZE * RTP_CHANNELS;
