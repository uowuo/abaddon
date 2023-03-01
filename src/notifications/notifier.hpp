#pragma once
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>

class Notifier {
public:
    Notifier();

    void Notify(const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action);
};
