#ifdef WITH_VOICE

// clang-format off

#include <spdlog/spdlog.h>

#include "abaddon.hpp"
#include "audio/manager.hpp"
#include "voicesettingswindow.hpp"

// clang-format on

using SignalHint = AbaddonClient::Audio::Voice::Opus::OpusEncoder::SignalHint;
using EncodingApplication = AbaddonClient::Audio::Voice::Opus::OpusEncoder::EncodingApplication;

VoiceSettingsWindow::VoiceSettingsWindow()
    : m_main(Gtk::ORIENTATION_VERTICAL) {
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("voice-settings-window");
    set_default_size(300, 300);

    m_encoding_mode.append("Voice");
    m_encoding_mode.append("Music");
    m_encoding_mode.append("Restricted");
    m_encoding_mode.set_tooltip_text(
        "Sets the coding mode for the Opus encoder\n"
        "Voice - Optimize for voice quality\n"
        "Music - Optimize for non-voice signals incl. music\n"
        "Restricted - Optimize for non-voice, low latency. Not recommended");

    const auto mode = Abaddon::Get().GetAudio().GetVoice().GetCapture().GetEncoder()->value().GetEncodingApplication();

    if (mode == EncodingApplication::VOIP) {
        m_encoding_mode.set_active(0);
    } else if (mode == EncodingApplication::Audio) {
        m_encoding_mode.set_active(1);
    } else if (mode == EncodingApplication::LowDelay) {
        m_encoding_mode.set_active(2);
    }

    m_encoding_mode.signal_changed().connect([this]() {
        const auto mode = m_encoding_mode.get_active_text();
        auto &encoder = Abaddon::Get().GetAudio().GetVoice().GetCapture().GetEncoder()->value();

        if (mode == "Voice") {
            encoder.SetEncodingApplication(EncodingApplication::VOIP);
        } else if (mode == "Music") {
            spdlog::get("audio")->debug("music/audio");
            encoder.SetEncodingApplication(EncodingApplication::Audio);
        } else if (mode == "Restricted") {
            encoder.SetEncodingApplication(EncodingApplication::LowDelay);
        }
    });

    m_signal.append("Auto");
    m_signal.append("Voice");
    m_signal.append("Music");
    m_signal.set_tooltip_text(
        "Signal hint. Tells Opus what the current signal is\n"
        "Auto - Let Opus figure it out\n"
        "Voice - Tell Opus it's a voice signal\n"
        "Music - Tell Opus it's a music signal");

    const auto signal = Abaddon::Get().GetAudio().GetVoice().GetCapture().GetEncoder()->value().GetSignalHint();
    if (signal == SignalHint::Auto) {
        m_signal.set_active(0);
    } else if (signal == SignalHint::Voice) {
        m_signal.set_active(1);
    } else if (signal == SignalHint::Music) {
        m_signal.set_active(2);
    }

    m_signal.signal_changed().connect([this]() {
        const auto signal = m_signal.get_active_text();
        auto encoder = Abaddon::Get().GetAudio().GetVoice().GetCapture().GetEncoder();
        if (!encoder->has_value()) {
            return;
        }

        if (signal == "Auto") {
            encoder->value().SetSignalHint(SignalHint::Auto);
        } else if (signal == "Voice") {
            encoder->value().SetSignalHint(SignalHint::Voice);
        } else if (signal == "Music") {
            encoder->value().SetSignalHint(SignalHint::Music);
        }
    });

    // exponential scale for bitrate because higher bitrates dont sound much different
    constexpr static auto MAX_BITRATE = 128000.0;
    constexpr static auto MIN_BITRATE = 2400.0;
    const auto bitrate_scale = [this](double value) -> double {
        value /= 100.0;
        return (MAX_BITRATE - MIN_BITRATE) * value * value * value + MIN_BITRATE;
    };

    const auto bitrate_scale_r = [this](double value) -> double {
        return 100.0 * std::cbrt((value - MIN_BITRATE) / (MAX_BITRATE - MIN_BITRATE));
    };

    m_bitrate.set_range(0.0, 100.0);
    m_bitrate.set_value_pos(Gtk::POS_TOP);
    m_bitrate.set_value(bitrate_scale_r(Abaddon::Get().GetAudio().GetVoice().GetCapture().GetEncoder()->value().GetBitrate()));
    m_bitrate.signal_format_value().connect([this, bitrate_scale](double value) {
        const auto scaled = bitrate_scale(value);
        if (value <= 99.9) {
            return Glib::ustring(std::to_string(static_cast<int>(scaled)));
        } else {
            return Glib::ustring("MAX");
        }
    });
    m_bitrate.signal_value_changed().connect([this, bitrate_scale]() {
        const auto value = m_bitrate.get_value();
        const auto scaled = bitrate_scale(value);

        auto& encoder = Abaddon::Get().GetAudio().GetVoice().GetCapture().GetEncoder()->value();
        if (value <= 99.9) {
            encoder.SetBitrate(static_cast<int>(scaled));
        } else {
            encoder.SetBitrate(MAX_BITRATE);
        }
    });

    m_gain.set_increments(1.0, 5.0);
    m_gain.set_range(0.0, 6969696969.0);
    m_gain.set_value(Abaddon::Get().GetAudio().GetVoice().GetCapture().Gain * 100.0);
    const auto cb = [this]() {
        spdlog::get("ui")->warn("emit");
        m_signal_gain.emit(m_gain.get_value() / 100.0);
    };
    // m_gain.signal_value_changed can be fired during destruction. thankfully signals are trackable
    m_gain.signal_value_changed().connect(sigc::track_obj(cb, *this, m_signal_gain));

    auto *layout = Gtk::make_managed<Gtk::HBox>();
    auto *labels = Gtk::make_managed<Gtk::VBox>();
    auto *widgets = Gtk::make_managed<Gtk::VBox>();
    layout->pack_start(*labels, false, true, 5);
    layout->pack_start(*widgets);
    labels->pack_start(*Gtk::make_managed<Gtk::Label>("Coding Mode", Gtk::ALIGN_END));
    labels->pack_start(*Gtk::make_managed<Gtk::Label>("Signal Hint", Gtk::ALIGN_END));
    labels->pack_start(*Gtk::make_managed<Gtk::Label>("Bitrate", Gtk::ALIGN_END));
    labels->pack_start(*Gtk::make_managed<Gtk::Label>("Gain", Gtk::ALIGN_END));
    widgets->pack_start(m_encoding_mode);
    widgets->pack_start(m_signal);
    widgets->pack_start(m_bitrate);
    widgets->pack_start(m_gain);

    m_main.add(*layout);
    add(m_main);
    show_all_children();

    // no need to bring in ManageHeapWindow, no user menu
    signal_hide().connect([this]() {
        delete this;
    });
}

VoiceSettingsWindow::type_signal_gain VoiceSettingsWindow::signal_gain() {
    return m_signal_gain;
}

#endif
