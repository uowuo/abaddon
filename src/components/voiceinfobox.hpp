#pragma once

#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

class VoiceInfoBox : public Gtk::Box {
public:
    VoiceInfoBox();

private:
    Gtk::Box m_left;
    Gtk::Label m_status;
    Gtk::Label m_location;

    Gtk::EventBox m_disconnect_ev;
    Gtk::Image m_disconnect_img;
};
