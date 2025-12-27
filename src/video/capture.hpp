#pragma once
#ifdef WITH_VIDEO

#include <sigc++/sigc++.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>

class VideoCapture {
public:
    VideoCapture();
    ~VideoCapture();

    // Start capturing from camera
    bool StartCameraCapture(const std::string &device = "");

    // Start capturing from screen
    // If geometry parameters are provided (all > 0), use them for FFmpeg
    // Otherwise, use default/automatic detection
    bool StartScreenCapture(int x = 0, int y = 0, int width = 0, int height = 0);

    // Stop capturing
    void Stop();

    // Force a keyframe (I-frame) on next frame
    void ForceKeyframe();

    // Check if capturing
    bool IsCapturing() const { return m_running.load(); }

    // Signal emitted when encoded packet is ready
    // Parameters: encoded packet data, timestamp (90000 Hz clock)
    using type_signal_packet = sigc::signal<void, std::vector<uint8_t>, uint32_t>;
    type_signal_packet signal_packet();

private:
    void CaptureThread();
    bool InitializeEncoder();
    void Cleanup();
    std::string GetDefaultCameraDevice();
    std::string GetDefaultScreenDevice();

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_force_keyframe{false};
    std::thread m_capture_thread;
    std::mutex m_encoder_mutex;

    struct AVFormatContext *m_format_ctx = nullptr;
    struct AVCodecContext *m_codec_ctx = nullptr;
    struct AVFrame *m_frame = nullptr;
    struct AVPacket *m_packet = nullptr;
    struct SwsContext *m_sws_ctx = nullptr;

    bool m_is_screen = false;
    std::string m_device;

    // Screen geometry for FFmpeg
    int m_screen_x = 0;
    int m_screen_y = 0;
    int m_screen_width = 0;
    int m_screen_height = 0;

    type_signal_packet m_signal_packet;
};

#endif
