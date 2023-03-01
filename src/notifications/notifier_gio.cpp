#include "notifier.hpp"
#include <giomm/notification.h>

Notifier::Notifier() {}

void Notifier::Notify(const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action) {
    auto n = Gio::Notification::create(title);
    n->set_body(text);
    n->set_default_action(default_action);
    Abaddon::Get().GetApp()->send_notification(n);
}
