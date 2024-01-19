#pragma once
#include <gdk/gdk.h>
// idk it wont let me forward declare

namespace EventsUtil {
unsigned shortcut_key(GdkEventKey *event);
}
