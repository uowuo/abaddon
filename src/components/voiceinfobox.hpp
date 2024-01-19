#pragma once

#ifdef WITH_VOICE

// clang-format off

#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

// clang-format on

class VoiceInfoBox : public Gtk::Box {
public:
    VoiceInfoBox();

private:
    void UpdateLocation();

    Gtk::Box m_left;
    Gtk::EventBox m_status_ev;
    Gtk::Label m_status;
    Gtk::Label m_location;

    Gtk::EventBox m_disconnect_ev;
    Gtk::Image m_disconnect_img;
};

#endif
