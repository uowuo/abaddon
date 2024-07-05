#pragma once

#include "components/lazyimage.hpp"
#include "components/volumemeter.hpp"
#include "discord/snowflake.hpp"

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/label.h>
#include <gtkmm/listboxrow.h>
#include <gtkmm/scale.h>

class VoiceWindowSpeakerListEntry : public Gtk::ListBoxRow {
public:
    VoiceWindowSpeakerListEntry(Snowflake id);

    void SetVolumeMeter(double frac);
    void RestoreGain(double frac);

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
    type_signal_mute_cs signal_mute_cs();
    type_signal_volume signal_volume();

private:
    type_signal_mute_cs m_signal_mute_cs;
    type_signal_volume m_signal_volume;
};
