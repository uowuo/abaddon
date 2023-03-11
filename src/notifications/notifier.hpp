#pragma once
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>

#ifdef ENABLE_NOTIFICATION_SOUNDS
    #include <miniaudio.h>
#endif

class Notifier {
public:
    Notifier();
    ~Notifier();

    void Notify(const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action, const std::string &icon_path);

private:
#ifdef WITH_MINIAUDIO
    ma_engine m_engine;
#endif
};
