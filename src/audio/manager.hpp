#pragma once
#ifdef WITH_MINIAUDIO
// clang-format off

#include <gtkmm/treemodel.h>
#include <vector>
#include <miniaudio.h>
#include <opus.h>
#include <sigc++/sigc++.h>
#include <spdlog/spdlog.h>

#include "devices.hpp"

#ifdef WITH_VOICE
#include "voice/voice_audio.hpp"
#endif

#include "miniaudio/ma_log.hpp"

// clang-format on

class AudioManager {
public:
    AudioManager(const Glib::ustring &backends_string, DiscordClient &discord);

    void FeedMeOpus(uint32_t ssrc, std::vector<uint8_t> &&data);

    void SetPlaybackDevice(const Gtk::TreeModel::iterator &iter);
    void SetCaptureDevice(const Gtk::TreeModel::iterator &iter);

    void SetCapture(bool capture);
    void SetPlayback(bool playback);

    void SetCaptureGate(double gate);
    void SetCaptureGain(double gain);
    double GetCaptureGate() const noexcept;
    double GetCaptureGain() const noexcept;

    void SetMuteSSRC(uint32_t ssrc, bool mute);
    void SetVolumeSSRC(uint32_t ssrc, double volume);
    double GetVolumeSSRC(uint32_t ssrc) const;

    void SetEncodingApplication(int application);
    int GetEncodingApplication();
    void SetSignalHint(int signal);
    int GetSignalHint();
    void SetBitrate(int bitrate);
    int GetBitrate();

    void Enumerate();

    bool OK() const;

    double GetCaptureVolumeLevel() const noexcept;
    double GetSSRCVolumeLevel(uint32_t ssrc) const noexcept;

    AudioDevices &GetDevices();

    uint32_t GetRTPTimestamp() const noexcept;

    enum class VADMethod {
        Gate,
        RNNoise,
    };

    void SetVADMethod(const std::string &method);
    void SetVADMethod(VADMethod method);
    VADMethod GetVADMethod() const;

    static std::vector<ma_backend> ParseBackendsList(const Glib::ustring &list);

#ifdef WITH_RNNOISE
    float GetCurrentVADProbability() const;
    double GetRNNProbThreshold() const;
    void SetRNNProbThreshold(double value);
    void SetSuppressNoise(bool value);
    bool GetSuppressNoise() const;
#endif

    void SetMixMono(bool value);
    bool GetMixMono() const;

private:
    bool DecayVolumeMeters();

    bool m_ok;

    std::optional<AbaddonClient::Audio::Context> m_context;

    AudioDevices m_devices;

#ifdef WITH_VOICE
    std::optional<AbaddonClient::Audio::VoiceAudio> m_voice;
#endif

    std::optional<AbaddonClient::Audio::Miniaudio::MaLog> m_ma_log;
    std::shared_ptr<spdlog::logger> m_log;

public:
    using type_signal_opus_packet = sigc::signal<void(int payload_size)>;
    AbaddonClient::Audio::Voice::VoiceCapture::CaptureSignal signal_opus_packet();
};
#endif
