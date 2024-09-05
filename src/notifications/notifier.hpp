#pragma once
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>

class Notifier {
public:
    void Notify(const Glib::ustring &id, const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action, const std::string &icon_path);
    void Withdraw(const Glib::ustring &id);
};
