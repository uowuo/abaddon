#ifdef WITH_VIDEO

#include "capture.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

VideoCapture::VideoCapture() {
    avdevice_register_all();
}

VideoCapture::~VideoCapture() {
    Stop();
    Cleanup();
}

bool VideoCapture::StartCameraCapture(const std::string &device) {
    if (m_running.load()) {
        Stop();
    }

    m_is_screen = false;
    m_device = device.empty() ? GetDefaultCameraDevice() : device;

    if (!InitializeEncoder()) {
        return false;
    }

    m_running = true;
    m_capture_thread = std::thread(&VideoCapture::CaptureThread, this);
    return true;
}

bool VideoCapture::StartScreenCapture(int x, int y, int width, int height) {
    if (m_running.load()) {
        Stop();
    }

    m_is_screen = true;
    m_device = GetDefaultScreenDevice();

    // Store geometry if provided (all values > 0)
    if (width > 0 && height > 0) {
        m_screen_x = x;
        m_screen_y = y;
        m_screen_width = width;
        m_screen_height = height;
    } else {
        // Reset to defaults (automatic detection)
        m_screen_x = 0;
        m_screen_y = 0;
        m_screen_width = 0;
        m_screen_height = 0;
    }

    if (!InitializeEncoder()) {
        return false;
    }

    m_running = true;
    m_capture_thread = std::thread(&VideoCapture::CaptureThread, this);
    return true;
}

void VideoCapture::Stop() {
    if (!m_running.load()) return;

    m_running = false;
    if (m_capture_thread.joinable()) {
        m_capture_thread.join();
    }

    Cleanup();
}

void VideoCapture::ForceKeyframe() {
    m_force_keyframe = true;
}

VideoCapture::type_signal_packet VideoCapture::signal_packet() {
    return m_signal_packet;
}

std::string VideoCapture::GetDefaultCameraDevice() {
#ifdef _WIN32
    return "video=Integrated Camera"; // Default Windows camera
#elif __APPLE__
    return "0"; // avfoundation device index
#else
    return "/dev/video0"; // v4l2 device
#endif
}

std::string VideoCapture::GetDefaultScreenDevice() {
#ifdef _WIN32
    return "desktop"; // gdigrab
#elif __APPLE__
    return "1"; // avfoundation screen index
#else
    // Get DISPLAY environment variable, default to :0.0
    const char *display = std::getenv("DISPLAY");
    if (display && strlen(display) > 0) {
        return std::string(display);
    }
    return ":0.0"; // x11grab default
#endif
}

bool VideoCapture::InitializeEncoder() {
    Cleanup();

    AVFormatContext *format_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *sws_ctx = nullptr;

    // Open input device
    const AVInputFormat *input_format = nullptr;
    AVDictionary *options = nullptr;

    if (m_is_screen) {
#ifdef _WIN32
        input_format = av_find_input_format("gdigrab");
        av_dict_set(&options, "framerate", "30", 0);

        // Use geometry if provided
        if (m_screen_width > 0 && m_screen_height > 0) {
            av_dict_set(&options, "offset_x", std::to_string(m_screen_x).c_str(), 0);
            av_dict_set(&options, "offset_y", std::to_string(m_screen_y).c_str(), 0);
            std::string video_size = std::to_string(m_screen_width) + "x" + std::to_string(m_screen_height);
            av_dict_set(&options, "video_size", video_size.c_str(), 0);
        } else {
            av_dict_set(&options, "offset_x", "0", 0);
            av_dict_set(&options, "offset_y", "0", 0);
        }
#elif __APPLE__
        input_format = av_find_input_format("avfoundation");
        // Format: "screen:audio" or just screen index
        // Note: avfoundation uses screen indices, geometry handling may need adjustment
        // For now, use default behavior if geometry not provided
#else
        input_format = av_find_input_format("x11grab");
        av_dict_set(&options, "framerate", "30", 0);

        // Use geometry if provided
        if (m_screen_width > 0 && m_screen_height > 0) {
            // Set video_size with provided dimensions
            std::string video_size = std::to_string(m_screen_width) + "x" + std::to_string(m_screen_height);
            av_dict_set(&options, "video_size", video_size.c_str(), 0);

            // Build input URL with offset: ":display.screen+X,Y"
            // Extract display part from m_device (e.g., ":0.0" or "hostname:0.0")
            std::string display_part = m_device;
            if (display_part.find(':') == std::string::npos) {
                display_part = ":" + display_part;
            }
            // Ensure screen number is present
            if (display_part.find('.') == std::string::npos) {
                display_part = display_part + ".0";
            }
            // Append offset
            m_device = display_part + "+" + std::to_string(m_screen_x) + "," + std::to_string(m_screen_y);
        } else {
            // Default behavior: use automatic detection
            av_dict_set(&options, "video_size", "1280x720", 0);
            // Ensure device format is correct for x11grab
            // Format should be "hostname:display.screen" or ":display.screen"
            // If device doesn't have ':', prepend it
            if (m_device.find(':') == std::string::npos) {
                m_device = ":" + m_device;
            }
            // If device doesn't have screen number, add .0
            if (m_device.find('.') == std::string::npos && m_device.back() != '0') {
                m_device = m_device + ".0";
            }
        }

        // Try to use XAUTHORITY if available
        const char *xauth = std::getenv("XAUTHORITY");
        if (xauth) {
            av_dict_set(&options, "xauth", xauth, 0);
        }
#endif
    } else {
#ifdef _WIN32
        input_format = av_find_input_format("dshow");
#elif __APPLE__
        input_format = av_find_input_format("avfoundation");
#else
        input_format = av_find_input_format("v4l2");
        av_dict_set(&options, "video_size", "1280x720", 0);
        av_dict_set(&options, "framerate", "30", 0);
#endif
    }

    int ret = avformat_open_input(&format_ctx, m_device.c_str(), input_format, &options);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        spdlog::get("video")->error("Failed to open input device: {} (error: {})", m_device, errbuf);
        if (m_is_screen) {
            spdlog::get("video")->warn("Screen capture requires X11 authorization. Try running: xhost +local:");
            spdlog::get("video")->warn("Or ensure XAUTHORITY environment variable is set correctly");
        }
        av_dict_free(&options);
        return false;
    }
    av_dict_free(&options);

    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        spdlog::get("video")->error("Failed to find stream info");
        avformat_close_input(&format_ctx);
        return false;
    }

    int video_stream_idx = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    if (video_stream_idx == -1) {
        spdlog::get("video")->error("No video stream found");
        avformat_close_input(&format_ctx);
        return false;
    }

    AVCodecParameters *codecpar = format_ctx->streams[video_stream_idx]->codecpar;

    // Find H.264 encoder
    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!encoder) {
        spdlog::get("video")->error("H.264 encoder not found");
        avformat_close_input(&format_ctx);
        return false;
    }

    codec_ctx = avcodec_alloc_context3(encoder);
    if (!codec_ctx) {
        spdlog::get("video")->error("Failed to allocate codec context");
        avformat_close_input(&format_ctx);
        return false;
    }

    // Configure encoder
    codec_ctx->width = 1280;
    codec_ctx->height = 720;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->time_base = {1, 30};
    codec_ctx->framerate = {30, 1};
    // Force more frequent keyframes for screen share to ensure visibility
    // Even if screen is static, we need regular keyframes so Discord can decode
    // For screen share, use smaller GOP to ensure viewers can join quickly
    codec_ctx->gop_size = m_is_screen ? 30 : 60; // Screen: 1 second, Camera: 2 seconds
    codec_ctx->keyint_min = m_is_screen ? 30 : 60; // Minimum keyframe interval
    codec_ctx->max_b_frames = 0; // CRITICAL: No B-frames for zero latency
    codec_ctx->bit_rate = 2000000; // 2 Mbps

    // Set encoder options for zero latency streaming
    av_opt_set(codec_ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(codec_ctx->priv_data, "repeat_headers", "1", 0);
    av_opt_set(codec_ctx->priv_data, "annexb", "1", 0);

    // CRITICAL: x264 parameters for zero latency
    // keyint: Maximum keyframe interval (same as gop_size)
    // min-keyint: Minimum keyframe interval (same as keyint_min)
    // scenecut=0: Disable scene change detection (reduces lag)
    std::string x264_params = "keyint=" + std::to_string(codec_ctx->gop_size) +
                              ":min-keyint=" + std::to_string(codec_ctx->keyint_min) +
                              ":scenecut=0";
    av_opt_set(codec_ctx->priv_data, "x264-params", x264_params.c_str(), 0);

    if (avcodec_open2(codec_ctx, encoder, nullptr) < 0) {
        spdlog::get("video")->error("Failed to open encoder");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }

    // Allocate frame
    frame = av_frame_alloc();
    if (!frame) {
        spdlog::get("video")->error("Failed to allocate frame");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }

    frame->format = codec_ctx->pix_fmt;
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    if (av_frame_get_buffer(frame, 0) < 0) {
        spdlog::get("video")->error("Failed to allocate frame buffer");
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }

    // Allocate packet
    packet = av_packet_alloc();
    if (!packet) {
        spdlog::get("video")->error("Failed to allocate packet");
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }

    // Initialize swscale context for format conversion
    AVCodecContext *decoder_ctx = avcodec_alloc_context3(avcodec_find_decoder(codecpar->codec_id));
    if (decoder_ctx) {
        avcodec_parameters_to_context(decoder_ctx, codecpar);
        avcodec_open2(decoder_ctx, avcodec_find_decoder(codecpar->codec_id), nullptr);
        sws_ctx = sws_getContext(
            decoder_ctx->width, decoder_ctx->height, decoder_ctx->pix_fmt,
            codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        avcodec_free_context(&decoder_ctx);
    }

    // Store pointers
    m_format_ctx = format_ctx;
    m_codec_ctx = codec_ctx;
    m_frame = frame;
    m_packet = packet;
    m_sws_ctx = sws_ctx;

    spdlog::get("video")->info("Video capture initialized: {}x{}", codec_ctx->width, codec_ctx->height);
    return true;
}

void VideoCapture::Cleanup() {
    std::lock_guard<std::mutex> lock(m_encoder_mutex);

    if (m_sws_ctx) {
        sws_freeContext(m_sws_ctx);
        m_sws_ctx = nullptr;
    }

    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }

    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if (m_codec_ctx) {
        avcodec_free_context(&m_codec_ctx);
        m_codec_ctx = nullptr;
    }

    if (m_format_ctx) {
        avformat_close_input(&m_format_ctx);
        m_format_ctx = nullptr;
    }
}

void VideoCapture::CaptureThread() {
    AVFormatContext *format_ctx = static_cast<AVFormatContext *>(m_format_ctx);
    AVCodecContext *codec_ctx = static_cast<AVCodecContext *>(m_codec_ctx);
    AVFrame *frame = static_cast<AVFrame *>(m_frame);
    AVPacket *packet = static_cast<AVPacket *>(m_packet);
    SwsContext *sws_ctx = static_cast<SwsContext *>(m_sws_ctx);

    int video_stream_idx = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    AVCodecContext *decoder_ctx = avcodec_alloc_context3(avcodec_find_decoder(format_ctx->streams[video_stream_idx]->codecpar->codec_id));
    avcodec_parameters_to_context(decoder_ctx, format_ctx->streams[video_stream_idx]->codecpar);
    avcodec_open2(decoder_ctx, avcodec_find_decoder(format_ctx->streams[video_stream_idx]->codecpar->codec_id), nullptr);

    AVFrame *decoded_frame = av_frame_alloc();
    AVPacket *input_packet = av_packet_alloc();

    uint32_t timestamp = 0;
    const uint32_t timestamp_increment = 3000; // 90000 / 30 = 3000
    uint32_t frame_count = 0; // Counter for forcing keyframes

    while (m_running.load()) {
        // Read frame from device
        int read_result = av_read_frame(format_ctx, input_packet);
        if (read_result < 0) {
            // #region agent log
            {
                std::ofstream log_file("/home/klepto/programacion/abaddon/.cursor/debug.log", std::ios::app);
                log_file << "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"A\",\"location\":\"capture.cpp:" << __LINE__ << "\",\"message\":\"av_read_frame failed\",\"data\":{\"error\":" << read_result << ",\"is_screen\":" << (m_is_screen ? "true" : "false") << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            }
            // #endregion
            break;
        }

        if (input_packet->stream_index == video_stream_idx) {
            // #region agent log
            {
                std::ofstream log_file("/home/klepto/programacion/abaddon/.cursor/debug.log", std::ios::app);
                log_file << "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"A\",\"location\":\"capture.cpp:" << __LINE__ << "\",\"message\":\"Received input packet\",\"data\":{\"input_size\":" << input_packet->size << ",\"stream_index\":" << input_packet->stream_index << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            }
            // #endregion
            // Decode frame
            int send_result = avcodec_send_packet(decoder_ctx, input_packet);
            if (send_result == 0) {
                int receive_result = avcodec_receive_frame(decoder_ctx, decoded_frame);
                // #region agent log
                {
                    std::ofstream log_file("/home/klepto/programacion/abaddon/.cursor/debug.log", std::ios::app);
                    log_file << "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"A\",\"location\":\"capture.cpp:" << __LINE__ << "\",\"message\":\"Decoder receive result\",\"data\":{\"receive_result\":" << receive_result << ",\"frame_width\":" << (receive_result == 0 ? decoded_frame->width : 0) << ",\"frame_height\":" << (receive_result == 0 ? decoded_frame->height : 0) << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
                }
                // #endregion
                while (receive_result == 0) {
                    // Convert to YUV420P
                    if (sws_ctx) {
                        sws_scale(sws_ctx,
                                  decoded_frame->data, decoded_frame->linesize, 0, decoded_frame->height,
                                  frame->data, frame->linesize);
                    }

                    // Force keyframe if requested
                    {
                        std::lock_guard<std::mutex> lock(m_encoder_mutex);
                        if (m_force_keyframe.load()) {
                            frame->pict_type = AV_PICTURE_TYPE_I;
                            m_force_keyframe = false;
                            frame_count = 0; // Reset counter after forced keyframe
                        } else {
                            // For screen share, force keyframe every 15 frames (0.5 seconds) to ensure visibility
                            // This prevents empty P-frames when screen is static and allows viewers to join quickly
                            // For camera, use every 30 frames (1 second) to balance quality and latency
                            const uint32_t keyframe_interval = m_is_screen ? 15 : 30;
                            if (frame_count % keyframe_interval == 0) {
                                frame->pict_type = AV_PICTURE_TYPE_I;
                            } else {
                                frame->pict_type = AV_PICTURE_TYPE_NONE;
                            }
                        }
                    }

                    // CRITICAL: Set PTS in codec time_base units (1/30)
                    // The encoder will convert this internally, but we need to track it
                    // for RTP timestamp calculation (which uses 90000 Hz clock)
                    frame->pts = timestamp;
                    timestamp += timestamp_increment;
                    frame_count++;

                    // Encode frame
                    int encode_send_result = avcodec_send_frame(codec_ctx, frame);
                    // #region agent log
                    {
                        std::ofstream log_file("/home/klepto/programacion/abaddon/.cursor/debug.log", std::ios::app);
                        log_file << "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"A\",\"location\":\"capture.cpp:" << __LINE__ << "\",\"message\":\"Encoder send frame\",\"data\":{\"encode_send_result\":" << encode_send_result << ",\"frame_pts\":" << frame->pts << ",\"pict_type\":" << static_cast<int>(frame->pict_type) << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
                    }
                    // #endregion
                    if (encode_send_result == 0) {
                        while (avcodec_receive_packet(codec_ctx, packet) == 0) {
                            // Emit signal with encoded packet
                            std::vector<uint8_t> packet_data(packet->data, packet->data + packet->size);

                            // CRITICAL: Convert packet PTS from codec time_base to RTP timestamp (90000 Hz)
                            // Codec time_base is 1/30, RTP video clock is 90000 Hz
                            // We use av_rescale_q to convert from codec time_base to RTP clock
                            const uint32_t rtp_timestamp = static_cast<uint32_t>(
                                av_rescale_q(packet->pts, codec_ctx->time_base, AVRational{1, 90000})
                            );

                            // #region agent log
                            {
                                std::ofstream log_file("/home/klepto/programacion/abaddon/.cursor/debug.log", std::ios::app);
                                log_file << "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"A\",\"location\":\"capture.cpp:" << __LINE__ << "\",\"message\":\"VideoCapture emitting packet\",\"data\":{\"packet_size\":" << packet_data.size() << ",\"packet_pts\":" << packet->pts << ",\"rtp_timestamp\":" << rtp_timestamp << ",\"is_screen\":" << (m_is_screen ? "true" : "false") << ",\"flags\":" << packet->flags << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
                            }
                            // #endregion
                            m_signal_packet.emit(packet_data, rtp_timestamp);
                            av_packet_unref(packet);
                        }
                    }
                    receive_result = avcodec_receive_frame(decoder_ctx, decoded_frame);
                }
            } else {
                // #region agent log
                {
                    std::ofstream log_file("/home/klepto/programacion/abaddon/.cursor/debug.log", std::ios::app);
                    log_file << "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"A\",\"location\":\"capture.cpp:" << __LINE__ << "\",\"message\":\"Decoder send failed\",\"data\":{\"send_result\":" << send_result << "},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
                }
                // #endregion
            }
        }

        av_packet_unref(input_packet);
    }

    av_frame_free(&decoded_frame);
    av_packet_free(&input_packet);
    avcodec_free_context(&decoder_ctx);
}

#endif
