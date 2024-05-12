#pragma once

#include <miniaudio.h>

constexpr ma_format CAPTURE_FORMAT = ma_format_f32;
constexpr uint32_t CAPTURE_SAMPLE_RATE = 48000;
constexpr uint32_t CAPTURE_CHANNELS = 2;
constexpr uint32_t CAPTURE_FRAME_SIZE = 480;
constexpr size_t CAPTURE_BUFFER_SIZE = CAPTURE_FRAME_SIZE * CAPTURE_CHANNELS;

using CaptureBuffer = std::array<float, CAPTURE_BUFFER_SIZE>;

constexpr size_t OPUS_LATENCY_TENTH_MS = 10000 * CAPTURE_FRAME_SIZE / CAPTURE_SAMPLE_RATE;
static_assert
(
    OPUS_LATENCY_TENTH_MS == 25  ||
    OPUS_LATENCY_TENTH_MS == 50  ||
    OPUS_LATENCY_TENTH_MS == 100 ||
    OPUS_LATENCY_TENTH_MS == 200 ||
    OPUS_LATENCY_TENTH_MS == 400 ||
    OPUS_LATENCY_TENTH_MS == 600,
    "Opus latency should be either 2.5, 5, 10, 20, 40 or 60 ms"
);
