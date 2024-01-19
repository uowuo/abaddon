#pragma once

#include <gtkmm/comboboxtext.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>

#include "discord/objects.hpp"

class SetStatusDialog : public Gtk::Dialog {
public:
    SetStatusDialog(Gtk::Window &parent);
    ActivityType GetActivityType() const;
    PresenceStatus GetStatusType() const;
    std::string GetActivityName() const;

private:
    Gtk::Box m_layout;
    Gtk::Entry m_text;
    Gtk::ComboBoxText m_status_combo;
    Gtk::ComboBoxText m_type_combo;

    Gtk::Button m_ok;
    Gtk::Button m_cancel;
};
