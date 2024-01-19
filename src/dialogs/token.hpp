#pragma once

#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>

class TokenDialog : public Gtk::Dialog {
public:
    TokenDialog(Gtk::Window &parent);
    std::string GetToken();

protected:
    Gtk::Box m_layout;
    Gtk::Button m_ok;
    Gtk::Button m_cancel;
    Gtk::ButtonBox m_bbox;
    Gtk::Entry m_entry;

private:
    std::string m_token;
};
