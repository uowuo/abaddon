#pragma once
#include "discord/objects.hpp"

class SetStatusDialog : public Gtk::Dialog {
public:
    SetStatusDialog(Gtk::Window &parent);
    ActivityType GetActivityType() const;
    PresenceStatus GetStatusType() const;
    std::string GetActivityName() const;

protected:
    Gtk::Box m_layout;
    Gtk::Box m_bottom;
    Gtk::Entry m_text;
    Gtk::ComboBoxText m_status_combo;
    Gtk::ComboBoxText m_type_combo;

    Gtk::Button m_ok;
    Gtk::Button m_cancel;
    Gtk::ButtonBox m_bbox;
};
