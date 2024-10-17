#include "notifier.hpp"

#include <giomm/notification.h>

#include "abaddon.hpp"

void Notifier::Notify(const Glib::ustring &id, const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action, const std::string &icon_path) {
    auto n = Gio::Notification::create(title);
    n->set_body(text);
    n->set_default_action(default_action);

    // i dont think giomm provides an interface for this

    auto *file = g_file_new_for_path(icon_path.c_str());
    auto *icon = g_file_icon_new(file);
    g_notification_set_icon(n->gobj(), icon);

    Abaddon::Get().GetApp()->send_notification(id, n);

    g_object_unref(icon);
    g_object_unref(file);

#ifdef ENABLE_NOTIFICATION_SOUNDS
    using SystemSound = AbaddonClient::Audio::SystemAudio::SystemSound;

    auto& abaddon = Abaddon::Get();
    if (abaddon.GetSettings().NotificationsPlaySound) {
        abaddon.GetAudio().GetSystem().PlaySound(SystemSound::Notification);
    }
#endif
}

void Notifier::Withdraw(const Glib::ustring &id) {
    Abaddon::Get().GetApp()->withdraw_notification(id);
}
