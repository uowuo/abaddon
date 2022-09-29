#pragma once
#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/window.h>

class VoiceWindow : public Gtk::Window {
public:
    VoiceWindow();

private:
    void OnMuteChanged();
    void OnDeafenChanged();

    Gtk::Box m_main;
    Gtk::Box m_controls;

    Gtk::CheckButton m_mute;
    Gtk::CheckButton m_deafen;

public:
    using type_signal_mute = sigc::signal<void(bool)>;
    using type_signal_deafen = sigc::signal<void(bool)>;

    type_signal_mute signal_mute();
    type_signal_deafen signal_deafen();

private:
    type_signal_mute m_signal_mute;
    type_signal_deafen m_signal_deafen;
};
