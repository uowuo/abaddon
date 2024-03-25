#pragma once
#include <chrono>
#include <cstdint>
#include <deque>

// very simple non-RTP-based jitter buffer. does not handle out-of-order
template<typename SampleFormat>
class JitterBuffer {
public:
    /*
     * desired_latency: how many milliseconds before audio can be drawn from buffer
     * maximum_latency: how many milliseconds before old audio starts to be discarded
     */
    JitterBuffer(int desired_latency, int maximum_latency, int channels, int sample_rate)
        : m_desired_latency(desired_latency)
        , m_maximum_latency(maximum_latency)
        , m_channels(channels)
        , m_sample_rate(sample_rate)
        , m_last_push(std::chrono::steady_clock::now()) {
    }

    [[nodiscard]] size_t Available() const noexcept {
        return m_samples.size();
    }

    bool PopSamples(SampleFormat *ptr, size_t amount) {
        CheckBuffering();
        if (m_buffering || Available() < amount) return false;
        std::copy(m_samples.begin(), m_samples.begin() + amount, ptr);
        m_samples.erase(m_samples.begin(), m_samples.begin() + amount);
        return true;
    }

    void PushSamples(SampleFormat *ptr, size_t amount) {
        m_samples.insert(m_samples.end(), ptr, ptr + amount);
        m_last_push = std::chrono::steady_clock::now();
        const auto buffered = MillisBuffered();
        if (buffered > m_maximum_latency) {
            const auto overflow_ms = MillisBuffered() - m_maximum_latency;
            const auto overflow_samples = overflow_ms * m_channels * m_sample_rate / 1000;
            m_samples.erase(m_samples.begin(), m_samples.begin() + overflow_samples);
        }
    }

private:
    [[nodiscard]] size_t MillisBuffered() const {
        return m_samples.size() * 1000 / m_channels / m_sample_rate;
    }

    void CheckBuffering() {
        // if we arent buffering but the buffer is empty then we should be
        if (m_samples.empty()) {
            if (!m_buffering) {
                m_buffering = true;
            }
            return;
        }

        if (!m_buffering) return;

        // if we reached desired latency, we are sufficiently buffered
        const auto millis_buffered = MillisBuffered();
        if (millis_buffered >= m_desired_latency) {
            m_buffering = false;
        }
        // if we havent buffered to desired latency but max latency has elapsed, exit buffering so it doesnt get stuck
        const auto now = std::chrono::steady_clock::now();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_push).count();
        if (millis >= m_maximum_latency) {
            m_buffering = false;
        }
    }

    int m_desired_latency;
    int m_maximum_latency;
    int m_channels;
    int m_sample_rate;
    bool m_buffering = true;
    std::chrono::time_point<std::chrono::steady_clock> m_last_push;

    std::deque<SampleFormat> m_samples;
};
