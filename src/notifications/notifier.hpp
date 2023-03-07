#pragma once
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>

#ifdef WITH_MINIAUDIO
#include <miniaudio.h>
#endif

class Notifier {
public:
    Notifier();
    ~Notifier();

    void Notify(const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action);

private:
#ifdef WITH_MINIAUDIO
    ma_engine m_engine;
#endif
};
