#pragma once
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

class VoiceWindowUserListEntry;
class VoiceWindow : public Gtk::Window {
public:
    VoiceWindow(Snowflake channel_id);

private:
    void SetUsers(const std::unordered_set<Snowflake> &user_ids);

    Gtk::ListBoxRow *CreateRow(Snowflake id);

    void OnUserConnect(Snowflake user_id, Snowflake to_channel_id);
    void OnUserDisconnect(Snowflake user_id, Snowflake from_channel_id);

    void OnMuteChanged();
    void OnDeafenChanged();

    bool UpdateVoiceMeters();

    void UpdateVADParamValue();

    Gtk::Box m_main;
    Gtk::Box m_controls;

    Gtk::CheckButton m_mute;
    Gtk::CheckButton m_deafen;

    Gtk::ScrolledWindow m_scroll;
    Gtk::ListBox m_user_list;

    // Shows volume for gate VAD method
    // Shows probability for RNNoise VAD method
    VolumeMeter m_vad_value;
    // Volume threshold for gate VAD method
    // VAD probability threshold for RNNoise VAD method
    Gtk::Scale m_vad_param;
    Gtk::Scale m_capture_gain;

    Gtk::CheckButton m_noise_suppression;
    Gtk::CheckButton m_mix_mono;

    Gtk::ComboBoxText m_vad_combo;
    Gtk::ComboBox m_playback_combo;
    Gtk::ComboBox m_capture_combo;

    Snowflake m_channel_id;

    std::unordered_map<Snowflake, VoiceWindowUserListEntry *> m_rows;

    Gtk::MenuBar m_menu_bar;
    Gtk::MenuItem m_menu_view;
    Gtk::Menu m_menu_view_sub;
    Gtk::MenuItem m_menu_view_settings;

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
