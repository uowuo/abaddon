#pragma once

#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>

class TextInputDialog : public Gtk::Dialog {
public:
    TextInputDialog(const Glib::ustring &prompt, const Glib::ustring &title, const Glib::ustring &placeholder, Gtk::Window &parent);

    Glib::ustring GetInput() const;

private:
    Gtk::Label m_label;
    Gtk::Entry m_entry;
};
