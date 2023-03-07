#include "notifier.hpp"
#include <giomm/notification.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

Notifier::Notifier() {
#ifdef WITH_MINIAUDIO
    if (ma_engine_init(nullptr, &m_engine) != MA_SUCCESS) {
        printf("failed to initialize miniaudio engine\n");
    }
#endif
}

Notifier::~Notifier() {
#ifdef WITH_MINIAUDIO
    ma_engine_uninit(&m_engine);
#endif
}

void Notifier::Notify(const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action) {
    auto n = Gio::Notification::create(title);
    n->set_body(text);
    n->set_default_action(default_action);
    Abaddon::Get().GetApp()->send_notification(n);

#ifdef WITH_MINIAUDIO
    ma_engine_play_sound(&m_engine, Abaddon::Get().GetResPath("/sound/message.mp3").c_str(), nullptr);
#endif
}
