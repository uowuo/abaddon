#pragma once

#include <algorithm>
#include <numeric>

#include "misc/slice.hpp"

using InputBuffer = ConstSlice<float>;
using OutputBuffer = Slice<float>;

namespace AbaddonClient::Audio {

class AudioUtils {
public:
    AudioUtils() = delete;
    ~AudioUtils() = delete;

    static void ApplyGain(OutputBuffer buffer, float gain) noexcept {
        for (auto &sample : buffer) {
            sample *= gain;
        }
    }

    static void ClampToFloatRange(OutputBuffer buffer) noexcept {
        for (auto& sample : buffer) {
            sample = std::clamp(sample, -1.0f, 1.0f);
        }
    }

    static void MixStereoToMono(OutputBuffer buffer) noexcept {
        for (auto iter = buffer.begin(); iter < buffer.end() - 2; iter += 2) {
            const auto mixed = std::reduce(iter, iter + 2, 0.0f) / 2.0f;
            std::fill(iter, iter + 2, mixed);
        }
    }

    static void MixBuffers(InputBuffer first, OutputBuffer second) noexcept {
        std::transform(first.begin(), first.end(), second.begin(), second.begin(), std::plus());
    }

};

}
