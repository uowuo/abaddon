#include "notifier.hpp"
#include <giomm/notification.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

Notifier::Notifier() {
#ifdef ENABLE_NOTIFICATION_SOUNDS
    if (ma_engine_init(nullptr, &m_engine) != MA_SUCCESS) {
        printf("failed to initialize miniaudio engine\n");
    }
#endif
}

Notifier::~Notifier() {
#ifdef ENABLE_NOTIFICATION_SOUNDS
    ma_engine_uninit(&m_engine);
#endif
}

void Notifier::Notify(const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action, const std::string &icon_path) {
    auto n = Gio::Notification::create(title);
    n->set_body(text);
    n->set_default_action(default_action);

    // i dont think giomm provides an interface for this

    auto *file = g_file_new_for_path(icon_path.c_str());
    auto *icon = g_file_icon_new(file);
    g_notification_set_icon(n->gobj(), icon);

    Abaddon::Get().GetApp()->send_notification(n);

    g_object_unref(icon);
    g_object_unref(file);

#ifdef ENABLE_NOTIFICATION_SOUNDS
    ma_engine_play_sound(&m_engine, Abaddon::Get().GetResPath("/sound/message.mp3").c_str(), nullptr);
#endif
}
