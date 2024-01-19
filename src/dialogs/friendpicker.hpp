#pragma once

#include <gtkmm/dialog.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>

#include "discord/snowflake.hpp"

class FriendPickerDialog : public Gtk::Dialog {
public:
    FriendPickerDialog(Gtk::Window &parent);

    Snowflake GetUserID() const;

protected:
    void OnRowActivated(Gtk::ListBoxRow *row);
    void OnSelectionChange();

    Snowflake m_chosen_id;

    Gtk::ScrolledWindow m_main;
    Gtk::ListBox m_list;
    Gtk::ButtonBox m_bbox;

    Gtk::Button *m_ok_button;
    Gtk::Button *m_cancel_button;
};

class FriendPickerDialogItem : public Gtk::ListBoxRow {
public:
    FriendPickerDialogItem(Snowflake user_id);

    Snowflake ID;

private:
    Gtk::Box m_layout;
    Gtk::Image m_avatar;
    Gtk::Label m_name;
};
