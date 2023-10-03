#include "events.hpp"

namespace EventsUtil {
unsigned shortcut_key(GdkEventKey *event) {
    // thanks inkscape
    unsigned shortcut_key = 0;
    gdk_keymap_translate_keyboard_state(
        gdk_keymap_get_for_display(gdk_display_get_default()),
        event->hardware_keycode,
        static_cast<GdkModifierType>(event->state),
        0,
        &shortcut_key, nullptr, nullptr, nullptr);
    return shortcut_key;
}
} // namespace EventsUtil
