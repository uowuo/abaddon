#ifdef WITH_VOICE

// clang-format off

#include "abaddon.hpp"
#include "audio/manager.hpp"
#include "components/lazyimage.hpp"
#include "voicesettingswindow.hpp"
#include "voicewindow.hpp"

// clang-format on

class VoiceWindowUserListEntry : public Gtk::ListBoxRow {
public:
    VoiceWindowUserListEntry(Snowflake id)
        : m_main(Gtk::ORIENTATION_VERTICAL)
        , m_horz(Gtk::ORIENTATION_HORIZONTAL)
        , m_avatar(32, 32)
        , m_mute("Mute") {
        m_name.set_halign(Gtk::ALIGN_START);
        m_name.set_hexpand(true);
        m_mute.set_halign(Gtk::ALIGN_END);

        m_volume.set_range(0.0, 200.0);
        m_volume.set_value_pos(Gtk::POS_LEFT);
        m_volume.set_value(100.0);
        m_volume.signal_value_changed().connect([this]() {
            m_signal_volume.emit(m_volume.get_value() * 0.01);
        });

        m_horz.add(m_avatar);
        m_horz.add(m_name);
        m_horz.add(m_mute);
        m_main.add(m_horz);
        m_main.add(m_volume);
        m_main.add(m_meter);
        add(m_main);
        show_all_children();

        auto &discord = Abaddon::Get().GetDiscordClient();
        const auto user = discord.GetUser(id);
        if (user.has_value()) {
            m_name.set_text(user->GetUsername());
            m_avatar.SetURL(user->GetAvatarURL("png", "32"));
        } else {
            m_name.set_text("Unknown user");
        }

        m_mute.signal_toggled().connect([this]() {
            m_signal_mute_cs.emit(m_mute.get_active());
        });
    }

    void SetVolumeMeter(double frac) {
        m_meter.SetVolume(frac);
    }

    void RestoreGain(double frac) {
        m_volume.set_value(frac * 100.0);
    }

private:
    Gtk::Box m_main;
    Gtk::Box m_horz;
    LazyImage m_avatar;
    Gtk::Label m_name;
    Gtk::CheckButton m_mute;
    Gtk::Scale m_volume;
    VolumeMeter m_meter;

public:
    using type_signal_mute_cs = sigc::signal<void(bool)>;
    using type_signal_volume = sigc::signal<void(double)>;
    type_signal_mute_cs signal_mute_cs() {
        return m_signal_mute_cs;
    }

    type_signal_volume signal_volume() {
        return m_signal_volume;
    }

private:
    type_signal_mute_cs m_signal_mute_cs;
    type_signal_volume m_signal_volume;
};

VoiceWindow::VoiceWindow(Snowflake channel_id)
    : m_main(Gtk::ORIENTATION_VERTICAL)
    , m_controls(Gtk::ORIENTATION_HORIZONTAL)
    , m_mute("Mute")
    , m_deafen("Deafen")
    , m_noise_suppression("Suppress Noise")
    , m_mix_mono("Mix Mono")
    , m_channel_id(channel_id)
    , m_menu_view("View")
    , m_menu_view_settings("More _Settings", true) {
    get_style_context()->add_class("app-window");

    set_default_size(300, 300);

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto &audio = Abaddon::Get().GetAudio();

    SetUsers(discord.GetUsersInVoiceChannel(m_channel_id));

    discord.signal_voice_user_disconnect().connect(sigc::mem_fun(*this, &VoiceWindow::OnUserDisconnect));
    discord.signal_voice_user_connect().connect(sigc::mem_fun(*this, &VoiceWindow::OnUserConnect));

    if (const auto self_state = discord.GetVoiceState(discord.GetUserData().ID); self_state.has_value()) {
        m_mute.set_active((self_state->second & VoiceStateFlags::SelfMute) == VoiceStateFlags::SelfMute);
        m_deafen.set_active((self_state->second & VoiceStateFlags::SelfDeaf) == VoiceStateFlags::SelfDeaf);
    }

    m_mute.signal_toggled().connect(sigc::mem_fun(*this, &VoiceWindow::OnMuteChanged));
    m_deafen.signal_toggled().connect(sigc::mem_fun(*this, &VoiceWindow::OnDeafenChanged));

    m_scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    m_scroll.set_hexpand(true);
    m_scroll.set_vexpand(true);

    m_vad_value.SetShowTick(true);

    m_vad_param.set_range(0.0, 100.0);
    m_vad_param.set_value_pos(Gtk::POS_LEFT);
    m_vad_param.signal_value_changed().connect([this]() {
        auto &audio = Abaddon::Get().GetAudio();
        const double val = m_vad_param.get_value() * 0.01;
        switch (audio.GetVADMethod()) {
            case AudioManager::VADMethod::Gate:
                audio.SetCaptureGate(val);
                m_vad_value.SetTick(val);
                break;
#ifdef WITH_RNNOISE
            case AudioManager::VADMethod::RNNoise:
                audio.SetRNNProbThreshold(val);
                m_vad_value.SetTick(val);
                break;
#endif
        };
    });
    UpdateVADParamValue();

    m_capture_gain.set_range(0.0, 200.0);
    m_capture_gain.set_value_pos(Gtk::POS_LEFT);
    m_capture_gain.set_value(audio.GetCaptureGain() * 100.0);
    m_capture_gain.signal_value_changed().connect([this]() {
        const double val = m_capture_gain.get_value() / 100.0;
        Abaddon::Get().GetAudio().SetCaptureGain(val);
    });

    m_vad_combo.set_valign(Gtk::ALIGN_END);
    m_vad_combo.set_hexpand(true);
    m_vad_combo.set_halign(Gtk::ALIGN_FILL);
    m_vad_combo.set_tooltip_text(
        "Voice Activation Detection method\n"
        "Gate - Simple volume threshold. Slider changes threshold\n"
        "RNNoise - Heavier on CPU. Slider changes probability threshold");
    m_vad_combo.append("gate", "Gate");
#ifdef WITH_RNNOISE
    m_vad_combo.append("rnnoise", "RNNoise");
#endif
    if (!m_vad_combo.set_active_id(Abaddon::Get().GetSettings().VAD)) {
#ifdef WITH_RNNOISE
        m_vad_combo.set_active_id("rnnoise");
#else
        m_vad_combo.set_active_id("gate");
#endif
    }
    m_vad_combo.signal_changed().connect([this]() {
        auto &audio = Abaddon::Get().GetAudio();
        const auto id = m_vad_combo.get_active_id();

        audio.SetVADMethod(id);
        Abaddon::Get().GetSettings().VAD = id;
        UpdateVADParamValue();
    });

    m_noise_suppression.set_active(audio.GetSuppressNoise());
    m_noise_suppression.signal_toggled().connect([this]() {
        Abaddon::Get().GetAudio().SetSuppressNoise(m_noise_suppression.get_active());
    });

    m_mix_mono.set_active(audio.GetMixMono());
    m_mix_mono.signal_toggled().connect([this]() {
        Abaddon::Get().GetAudio().SetMixMono(m_mix_mono.get_active());
    });

    auto *playback_renderer = Gtk::make_managed<Gtk::CellRendererText>();
    m_playback_combo.set_valign(Gtk::ALIGN_END);
    m_playback_combo.set_hexpand(true);
    m_playback_combo.set_halign(Gtk::ALIGN_FILL);
    m_playback_combo.set_model(audio.GetDevices().GetPlaybackDeviceModel());
    if (const auto iter = audio.GetDevices().GetActivePlaybackDevice()) {
        m_playback_combo.set_active(iter);
    }
    m_playback_combo.pack_start(*playback_renderer);
    m_playback_combo.add_attribute(*playback_renderer, "text", 0);
    m_playback_combo.signal_changed().connect([this]() {
        Abaddon::Get().GetAudio().SetPlaybackDevice(m_playback_combo.get_active());
    });

    auto *capture_renderer = Gtk::make_managed<Gtk::CellRendererText>();
    m_capture_combo.set_valign(Gtk::ALIGN_END);
    m_capture_combo.set_hexpand(true);
    m_capture_combo.set_halign(Gtk::ALIGN_FILL);
    m_capture_combo.set_model(Abaddon::Get().GetAudio().GetDevices().GetCaptureDeviceModel());
    if (const auto iter = Abaddon::Get().GetAudio().GetDevices().GetActiveCaptureDevice()) {
        m_capture_combo.set_active(iter);
    }
    m_capture_combo.pack_start(*capture_renderer);
    m_capture_combo.add_attribute(*capture_renderer, "text", 0);
    m_capture_combo.signal_changed().connect([this]() {
        Abaddon::Get().GetAudio().SetCaptureDevice(m_capture_combo.get_active());
    });

    m_menu_bar.append(m_menu_view);
    m_menu_view.set_submenu(m_menu_view_sub);
    m_menu_view_sub.append(m_menu_view_settings);
    m_menu_view_settings.signal_activate().connect([this]() {
        auto *window = new VoiceSettingsWindow;
        const auto cb = [this](double gain) {
            m_capture_gain.set_value(gain * 100.0);
            Abaddon::Get().GetAudio().SetCaptureGain(gain);
        };
        window->signal_gain().connect(sigc::track_obj(cb, *this));
        window->show();
    });

    auto *sliders_container = Gtk::make_managed<Gtk::HBox>();
    auto *sliders_labels = Gtk::make_managed<Gtk::VBox>();
    auto *sliders_sliders = Gtk::make_managed<Gtk::VBox>();
    sliders_container->pack_start(*sliders_labels, false, true, 2);
    sliders_container->pack_start(*sliders_sliders);
    sliders_labels->pack_start(*Gtk::make_managed<Gtk::Label>("Threshold", Gtk::ALIGN_END));
    sliders_labels->pack_start(*Gtk::make_managed<Gtk::Label>("Gain", Gtk::ALIGN_END));
    sliders_sliders->pack_start(m_vad_param);
    sliders_sliders->pack_start(m_capture_gain);

    auto *combos_container = Gtk::make_managed<Gtk::HBox>();
    auto *combos_labels = Gtk::make_managed<Gtk::VBox>();
    auto *combos_combos = Gtk::make_managed<Gtk::VBox>();
    combos_container->pack_start(*combos_labels, false, true, 6);
    combos_container->pack_start(*combos_combos, Gtk::PACK_EXPAND_WIDGET, 6);
    combos_labels->pack_start(*Gtk::make_managed<Gtk::Label>("VAD Method", Gtk::ALIGN_END));
    combos_labels->pack_start(*Gtk::make_managed<Gtk::Label>("Output Device", Gtk::ALIGN_END));
    combos_labels->pack_start(*Gtk::make_managed<Gtk::Label>("Input Device", Gtk::ALIGN_END));
    combos_combos->pack_start(m_vad_combo);
    combos_combos->pack_start(m_playback_combo);
    combos_combos->pack_start(m_capture_combo);

    m_scroll.add(m_user_list);
    m_controls.add(m_mute);
    m_controls.add(m_deafen);
    m_controls.add(m_noise_suppression);
    m_controls.add(m_mix_mono);
    m_main.pack_start(m_menu_bar, false, true);
    m_main.pack_start(m_controls, false, true);
    m_main.pack_start(m_vad_value, false, true);
    m_main.pack_start(*Gtk::make_managed<Gtk::Label>("Input Settings"), false, true);
    m_main.pack_start(*sliders_container, false, true);
    m_main.pack_start(m_scroll);
    m_main.pack_start(*combos_container, false, true, 2);
    add(m_main);
    show_all_children();

    Glib::signal_timeout().connect(sigc::mem_fun(*this, &VoiceWindow::UpdateVoiceMeters), 40);
}

void VoiceWindow::SetUsers(const std::unordered_set<Snowflake> &user_ids) {
    const auto me = Abaddon::Get().GetDiscordClient().GetUserData().ID;
    for (auto id : user_ids) {
        if (id == me) continue;
        m_user_list.add(*CreateRow(id));
    }
}

Gtk::ListBoxRow *VoiceWindow::CreateRow(Snowflake id) {
    auto *row = Gtk::make_managed<VoiceWindowUserListEntry>(id);
    m_rows[id] = row;
    auto &vc = Abaddon::Get().GetDiscordClient().GetVoiceClient();
    row->RestoreGain(vc.GetUserVolume(id));
    row->signal_mute_cs().connect([this, id](bool is_muted) {
        m_signal_mute_user_cs.emit(id, is_muted);
    });
    row->signal_volume().connect([this, id](double volume) {
        m_signal_user_volume_changed.emit(id, volume);
    });
    row->show_all();
    return row;
}

void VoiceWindow::OnMuteChanged() {
    m_signal_mute.emit(m_mute.get_active());
}

void VoiceWindow::OnDeafenChanged() {
    m_signal_deafen.emit(m_deafen.get_active());
}

bool VoiceWindow::UpdateVoiceMeters() {
    auto &audio = Abaddon::Get().GetAudio();
    switch (audio.GetVADMethod()) {
        case AudioManager::VADMethod::Gate:
            m_vad_value.SetVolume(audio.GetCaptureVolumeLevel());
            break;
#ifdef WITH_RNNOISE
        case AudioManager::VADMethod::RNNoise:
            m_vad_value.SetVolume(audio.GetCurrentVADProbability());
            break;
#endif
    }

    for (auto [id, row] : m_rows) {
        const auto ssrc = Abaddon::Get().GetDiscordClient().GetSSRCOfUser(id);
        if (ssrc.has_value()) {
            row->SetVolumeMeter(audio.GetSSRCVolumeLevel(*ssrc));
        }
    }
    return true;
}

void VoiceWindow::UpdateVADParamValue() {
    auto &audio = Abaddon::Get().GetAudio();
    switch (audio.GetVADMethod()) {
        case AudioManager::VADMethod::Gate:
            m_vad_param.set_value(audio.GetCaptureGate() * 100.0);
            break;
#ifdef WITH_RNNOISE
        case AudioManager::VADMethod::RNNoise:
            m_vad_param.set_value(audio.GetRNNProbThreshold() * 100.0);
            break;
#endif
    }
}

void VoiceWindow::OnUserConnect(Snowflake user_id, Snowflake to_channel_id) {
    if (m_channel_id == to_channel_id) {
        if (auto it = m_rows.find(user_id); it == m_rows.end()) {
            m_user_list.add(*CreateRow(user_id));
        }
    }
}

void VoiceWindow::OnUserDisconnect(Snowflake user_id, Snowflake from_channel_id) {
    if (m_channel_id == from_channel_id) {
        if (auto it = m_rows.find(user_id); it != m_rows.end()) {
            delete it->second;
            m_rows.erase(it);
        }
    }
}

VoiceWindow::type_signal_mute VoiceWindow::signal_mute() {
    return m_signal_mute;
}

VoiceWindow::type_signal_deafen VoiceWindow::signal_deafen() {
    return m_signal_deafen;
}

VoiceWindow::type_signal_mute_user_cs VoiceWindow::signal_mute_user_cs() {
    return m_signal_mute_user_cs;
}

VoiceWindow::type_signal_user_volume_changed VoiceWindow::signal_user_volume_changed() {
    return m_signal_user_volume_changed;
}
#endif
