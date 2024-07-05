#include "util.hpp"
#ifdef WITH_VOICE

// clang-format off

#include "voicewindow.hpp"

#include "abaddon.hpp"
#include "audio/manager.hpp"
#include "components/lazyimage.hpp"
#include "voicewindowaudiencelistentry.hpp"
#include "voicewindowspeakerlistentry.hpp"
#include "windows/voicesettingswindow.hpp"

// clang-format on

VoiceWindow::VoiceWindow(Snowflake channel_id)
    : m_main(Gtk::ORIENTATION_VERTICAL)
    , m_controls(Gtk::ORIENTATION_HORIZONTAL)
    , m_mute("Mute")
    , m_deafen("Deafen")
    , m_noise_suppression("Suppress Noise")
    , m_mix_mono("Mix Mono")
    , m_stage_command("Request to Speak")
    , m_disconnect("Disconnect")
    , m_stage_invite_lbl("You've been invited to speak")
    , m_stage_accept("Accept")
    , m_stage_decline("Decline")
    , m_channel_id(channel_id)
    , m_menu_view("View")
    , m_menu_view_settings("More _Settings", true) {
    get_style_context()->add_class("app-window");

    set_default_size(300, 400);

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto &audio = Abaddon::Get().GetAudio();

    const auto channel = discord.GetChannel(m_channel_id);
    m_is_stage = channel.has_value() && channel->Type == ChannelType::GUILD_STAGE_VOICE;

    SetUsers(discord.GetUsersInVoiceChannel(m_channel_id));

    discord.signal_voice_user_disconnect().connect(sigc::mem_fun(*this, &VoiceWindow::OnUserDisconnect));
    discord.signal_voice_user_connect().connect(sigc::mem_fun(*this, &VoiceWindow::OnUserConnect));
    discord.signal_voice_speaker_state_changed().connect(sigc::mem_fun(*this, &VoiceWindow::OnSpeakerStateChanged));
    discord.signal_voice_state_set().connect(sigc::mem_fun(*this, &VoiceWindow::OnVoiceStateUpdate));

    if (const auto self_state = discord.GetVoiceState(discord.GetUserData().ID); self_state.has_value()) {
        m_mute.set_active(util::FlagSet(self_state->second.Flags, VoiceStateFlags::SelfMute));
        m_deafen.set_active(util::FlagSet(self_state->second.Flags, VoiceStateFlags::SelfDeaf));
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

    m_disconnect.signal_clicked().connect([this]() {
        Abaddon::Get().GetDiscordClient().DisconnectFromVoice();
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

    if (const auto instance = discord.GetStageInstanceFromChannel(channel_id); instance.has_value()) {
        m_stage_topic_label.show();
        UpdateStageTopicLabel(instance->Topic);
    } else {
        m_stage_topic_label.hide();
    }

    discord.signal_stage_instance_create().connect(sigc::mem_fun(*this, &VoiceWindow::OnStageInstanceCreate));
    discord.signal_stage_instance_update().connect(sigc::mem_fun(*this, &VoiceWindow::OnStageInstanceUpdate));
    discord.signal_stage_instance_delete().connect(sigc::mem_fun(*this, &VoiceWindow::OnStageInstanceDelete));

    m_stage_command.signal_clicked().connect([this]() {
        auto &discord = Abaddon::Get().GetDiscordClient();
        const auto user_id = discord.GetUserData().ID;
        const bool is_moderator = discord.IsStageModerator(user_id, m_channel_id);
        const bool is_speaker = discord.IsUserSpeaker(user_id);
        const bool is_invited_to_speak = discord.IsUserInvitedToSpeak(user_id);

        if (is_speaker) {
            discord.SetStageSpeaking(m_channel_id, false, NOOP_CALLBACK);
        } else if (is_moderator) {
            discord.SetStageSpeaking(m_channel_id, true, NOOP_CALLBACK);
        } else if (is_invited_to_speak) {
            discord.DeclineInviteToSpeak(m_channel_id, NOOP_CALLBACK);
        } else {
            const bool requested = discord.HasUserRequestedToSpeak(user_id);
            discord.RequestToSpeak(m_channel_id, !requested, NOOP_CALLBACK);
        }
    });

    m_stage_accept.signal_clicked().connect([this]() {
        Abaddon::Get().GetDiscordClient().SetStageSpeaking(m_channel_id, true, NOOP_CALLBACK);
    });

    m_stage_decline.signal_clicked().connect([this]() {
        Abaddon::Get().GetDiscordClient().DeclineInviteToSpeak(m_channel_id, NOOP_CALLBACK);
    });

    m_speakers_label.set_markup("<b>Speakers</b>");
    if (m_is_stage) m_listing.pack_start(m_speakers_label, false, true);
    m_listing.pack_start(m_speakers_list, false, true);
    m_audience_label.set_markup("<b>Audience</b>");
    if (m_is_stage) m_listing.pack_start(m_audience_label, false, true);
    if (m_is_stage) m_listing.pack_start(m_audience_list, false, true);
    m_scroll.add(m_listing);
    m_controls.add(m_mute);
    m_controls.add(m_deafen);
    m_controls.add(m_noise_suppression);
    m_controls.add(m_mix_mono);
    m_buttons.set_halign(Gtk::ALIGN_CENTER);
    if (m_is_stage) m_buttons.pack_start(m_stage_command, false, true);
    m_buttons.pack_start(m_disconnect, false, true);
    m_stage_invite_box.pack_start(m_stage_invite_lbl, false, true);
    m_stage_invite_box.pack_start(m_stage_invite_btns);
    m_stage_invite_btns.set_halign(Gtk::ALIGN_CENTER);
    m_stage_invite_btns.pack_start(m_stage_accept, false, true);
    m_stage_invite_btns.pack_start(m_stage_decline, false, true);
    m_main.pack_start(m_menu_bar, false, true);
    m_main.pack_start(m_controls, false, true);
    m_main.pack_start(m_buttons, false, true);
    m_main.pack_start(m_stage_invite_box, false, true);
    m_main.pack_start(m_vad_value, false, true);
    m_main.pack_start(*Gtk::make_managed<Gtk::Label>("Input Settings"), false, true);
    m_main.pack_start(*sliders_container, false, true);
    m_main.pack_start(m_scroll);
    m_stage_topic_label.set_ellipsize(Pango::ELLIPSIZE_END);
    m_stage_topic_label.set_halign(Gtk::ALIGN_CENTER);
    m_main.pack_start(m_stage_topic_label, false, true);
    m_main.pack_start(*combos_container, false, true, 2);
    add(m_main);
    show_all_children();

    Glib::signal_timeout().connect(sigc::mem_fun(*this, &VoiceWindow::UpdateVoiceMeters), 40);

    UpdateStageCommand();
}

void VoiceWindow::SetUsers(const std::unordered_set<Snowflake> &user_ids) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto me = discord.GetUserData().ID;
    for (auto id : user_ids) {
        if (!m_is_stage || discord.IsUserSpeaker(id)) {
            if (id != me) m_speakers_list.add(*CreateSpeakerRow(id));
        } else {
            m_audience_list.add(*CreateAudienceRow(id));
        }
    }
}

Gtk::ListBoxRow *VoiceWindow::CreateSpeakerRow(Snowflake id) {
    auto *row = Gtk::make_managed<VoiceWindowSpeakerListEntry>(id);
    m_rows[id] = row;
    auto &vc = Abaddon::Get().GetDiscordClient().GetVoiceClient();
    row->RestoreGain(vc.GetUserVolume(id));
    row->signal_mute_cs().connect([this, id](bool is_muted) {
        m_signal_mute_user_cs.emit(id, is_muted);
    });
    row->signal_volume().connect([this, id](double volume) {
        m_signal_user_volume_changed.emit(id, volume);
    });
    row->show();
    return row;
}

Gtk::ListBoxRow *VoiceWindow::CreateAudienceRow(Snowflake id) {
    auto *row = Gtk::make_managed<VoiceWindowAudienceListEntry>(id);
    m_rows[id] = row;
    row->show();
    return row;
}

void VoiceWindow::OnMuteChanged() {
    m_signal_mute.emit(m_mute.get_active());
}

void VoiceWindow::OnDeafenChanged() {
    m_signal_deafen.emit(m_deafen.get_active());
}

void VoiceWindow::TryDeleteRow(Snowflake id) {
    if (auto it = m_rows.find(id); it != m_rows.end()) {
        delete it->second;
        m_rows.erase(it);
    }
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
            if (auto *speaker_row = dynamic_cast<VoiceWindowSpeakerListEntry *>(row)) {
                speaker_row->SetVolumeMeter(audio.GetSSRCVolumeLevel(*ssrc));
            }
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

void VoiceWindow::UpdateStageCommand() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto user_id = discord.GetUserData().ID;

    m_has_requested_to_speak = discord.HasUserRequestedToSpeak(user_id);
    const bool is_moderator = discord.IsStageModerator(user_id, m_channel_id);
    const bool is_speaker = discord.IsUserSpeaker(user_id);
    const bool is_invited_to_speak = discord.IsUserInvitedToSpeak(user_id);

    m_stage_invite_box.set_visible(is_invited_to_speak);

    if (is_speaker) {
        m_stage_command.set_label("Leave the Stage");
    } else if (is_moderator) {
        m_stage_command.set_label("Speak on Stage");
    } else if (m_has_requested_to_speak) {
        m_stage_command.set_label("Cancel Request");
    } else if (is_invited_to_speak) {
        m_stage_command.set_label("Decline Invite");
    } else {
        m_stage_command.set_label("Request to Speak");
    }
}

void VoiceWindow::UpdateStageTopicLabel(const std::string &topic) {
    m_stage_topic_label.set_markup("Topic: " + topic);
}

void VoiceWindow::OnUserConnect(Snowflake user_id, Snowflake to_channel_id) {
    if (m_channel_id == to_channel_id) {
        if (auto it = m_rows.find(user_id); it == m_rows.end()) {
            if (Abaddon::Get().GetDiscordClient().IsUserSpeaker(user_id)) {
                m_speakers_list.add(*CreateSpeakerRow(user_id));
            } else {
                m_audience_list.add(*CreateAudienceRow(user_id));
            }
        }
    }
}

void VoiceWindow::OnUserDisconnect(Snowflake user_id, Snowflake from_channel_id) {
    if (m_channel_id == from_channel_id) TryDeleteRow(user_id);
}

void VoiceWindow::OnSpeakerStateChanged(Snowflake channel_id, Snowflake user_id, bool is_speaker) {
    if (m_channel_id != channel_id) return;
    TryDeleteRow(user_id);
    if (is_speaker) {
        m_speakers_list.add(*CreateSpeakerRow(user_id));
    } else {
        m_audience_list.add(*CreateAudienceRow(user_id));
    }
}

void VoiceWindow::OnVoiceStateUpdate(Snowflake user_id, Snowflake channel_id, VoiceStateFlags flags) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (user_id != discord.GetUserData().ID) return;

    UpdateStageCommand();
}

void VoiceWindow::OnStageInstanceCreate(const StageInstance &instance) {
    m_stage_topic_label.show();
    UpdateStageTopicLabel(instance.Topic);
}

void VoiceWindow::OnStageInstanceUpdate(const StageInstance &instance) {
    UpdateStageTopicLabel(instance.Topic);
}

void VoiceWindow::OnStageInstanceDelete(const StageInstance &instance) {
    m_stage_topic_label.hide();
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
