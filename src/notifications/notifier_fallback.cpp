#include "notifier.hpp"

#include "abaddon.hpp"

/* no actual notifications, just sounds
  GNotification has no win32 backend, and WinToast uses headers msys2 doesnt provide
  maybe it can be LoadLibrary'd in :s
*/

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

void Notifier::Notify(const Glib::ustring &id, const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action, const std::string &icon_path) {
#ifdef ENABLE_NOTIFICATION_SOUNDS
    if (Abaddon::Get().GetSettings().NotificationsPlaySound) {
        ma_engine_play_sound(&m_engine, Abaddon::Get().GetResPath("/sound/message.mp3").c_str(), nullptr);
    }
#endif
}

void Notifier::Withdraw(const Glib::ustring &id) {}
