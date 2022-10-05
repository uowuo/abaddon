#pragma once
#ifdef WITH_VOICE
// clang-format off

#include "discord/snowflake.hpp"
#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/listbox.h>
#include <gtkmm/window.h>
#include <unordered_set>
// clang-format on

class VoiceWindow : public Gtk::Window {
public:
    VoiceWindow(Snowflake channel_id);

private:
    void SetUsers(const std::unordered_set<Snowflake> &user_ids);

    void OnMuteChanged();
    void OnDeafenChanged();

    Gtk::Box m_main;
    Gtk::Box m_controls;

    Gtk::CheckButton m_mute;
    Gtk::CheckButton m_deafen;

    Gtk::ListBox m_user_list;

    Snowflake m_channel_id;

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
