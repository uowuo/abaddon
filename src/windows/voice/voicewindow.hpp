#pragma once
#include "discord/stage.hpp"
#include "discord/voicestate.hpp"
#ifdef WITH_VOICE
// clang-format off

#include "components/volumemeter.hpp"
#include "discord/snowflake.hpp"
#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/listbox.h>
#include <gtkmm/menubar.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/scale.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/window.h>
#include <unordered_set>
// clang-format on

class VoiceWindow : public Gtk::Window {
public:
    VoiceWindow(Snowflake channel_id);

private:
    void SetUsers(const std::unordered_set<Snowflake> &user_ids);

    Gtk::ListBoxRow *CreateSpeakerRow(Snowflake id);
    Gtk::ListBoxRow *CreateAudienceRow(Snowflake id);

    void OnUserConnect(Snowflake user_id, Snowflake to_channel_id);
    void OnUserDisconnect(Snowflake user_id, Snowflake from_channel_id);
    void OnSpeakerStateChanged(Snowflake channel_id, Snowflake user_id, bool is_speaker);
    void OnVoiceStateUpdate(Snowflake user_id, Snowflake channel_id, VoiceStateFlags flags);
    void OnStageInstanceCreate(const StageInstance &instance);
    void OnStageInstanceUpdate(const StageInstance &instance);
    void OnStageInstanceDelete(const StageInstance &instance);

    void OnMuteChanged();
    void OnDeafenChanged();

    void TryDeleteRow(Snowflake id);
    bool UpdateVoiceMeters();
    void UpdateVADParamValue();
    void UpdateStageCommand();
    void UpdateStageTopicLabel(const std::string &topic);

    Gtk::Box m_main;
    Gtk::Box m_controls;

    Gtk::CheckButton m_mute;
    Gtk::CheckButton m_deafen;

    Gtk::ScrolledWindow m_scroll;
    Gtk::VBox m_listing;
    Gtk::ListBox m_speakers_list;
    Gtk::ListBox m_audience_list;

    // Shows volume for gate VAD method
    // Shows probability for RNNoise VAD method
    VolumeMeter m_vad_value;
    // Volume threshold for gate VAD method
    // VAD probability threshold for RNNoise VAD method
    Gtk::Scale m_vad_param;
    Gtk::Scale m_capture_gain;

    Gtk::CheckButton m_noise_suppression;
    Gtk::CheckButton m_mix_mono;

    Gtk::HBox m_buttons;
    Gtk::Button m_disconnect;
    Gtk::Button m_stage_command;

    Gtk::VBox m_stage_invite_box;
    Gtk::Label m_stage_invite_lbl;
    Gtk::HBox m_stage_invite_btns;
    Gtk::Button m_stage_accept;
    Gtk::Button m_stage_decline;

    bool m_has_requested_to_speak = false;

    Gtk::ComboBoxText m_vad_combo;
    Gtk::ComboBox m_playback_combo;
    Gtk::ComboBox m_capture_combo;

    Snowflake m_channel_id;
    bool m_is_stage;

    std::unordered_map<Snowflake, Gtk::ListBoxRow *> m_rows;

    Gtk::MenuBar m_menu_bar;
    Gtk::MenuItem m_menu_view;
    Gtk::Menu m_menu_view_sub;
    Gtk::MenuItem m_menu_view_settings;

    Gtk::Label m_stage_topic_label;
    Gtk::Label m_speakers_label;
    Gtk::Label m_audience_label;

public:
    using type_signal_mute = sigc::signal<void(bool)>;
    using type_signal_deafen = sigc::signal<void(bool)>;
    using type_signal_mute_user_cs = sigc::signal<void(Snowflake, bool)>;
    using type_signal_user_volume_changed = sigc::signal<void(Snowflake, double)>;

    type_signal_mute signal_mute();
    type_signal_deafen signal_deafen();
    type_signal_mute_user_cs signal_mute_user_cs();
    type_signal_user_volume_changed signal_user_volume_changed();

private:
    type_signal_mute m_signal_mute;
    type_signal_deafen m_signal_deafen;
    type_signal_mute_user_cs m_signal_mute_user_cs;
    type_signal_user_volume_changed m_signal_user_volume_changed;
};
#endif
