#pragma once
#ifdef WITH_VOICE

// clang-format off

#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/scale.h>
#include <gtkmm/window.h>

// clang-format on

class VoiceSettingsWindow : public Gtk::Window {
public:
    VoiceSettingsWindow();

    Gtk::Box m_main;
    Gtk::ComboBoxText m_encoding_mode;
    Gtk::ComboBoxText m_signal;
    Gtk::Scale m_bitrate;

private:
};

#endif
