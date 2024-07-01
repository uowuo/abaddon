#include "notifier.hpp"

#include "abaddon.hpp"

/* no actual notifications, just sounds
  GNotification has no win32 backend, and WinToast uses headers msys2 doesnt provide
  maybe it can be LoadLibrary'd in :s
*/

void Notifier::Notify(const Glib::ustring &id, const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action, const std::string &icon_path) {
#ifdef ENABLE_NOTIFICATION_SOUNDS
    using SystemSound = AbaddonClient::Audio::SystemAudio::SystemSound;

    auto& abaddon = Abaddon::Get();
    if (abaddon.GetSettings().NotificationsPlaySound) {
        abaddon.GetAudio().GetSystem().PlaySound(SystemSound::Notification);
    }
#endif
}

void Notifier::Withdraw(const Glib::ustring &id) {}
