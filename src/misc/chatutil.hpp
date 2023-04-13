#pragma once
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include "discord/snowflake.hpp"

namespace Gtk {
class TextBuffer;
class TextView;
} // namespace Gtk

namespace ChatUtil {
Glib::ustring GetText(const Glib::RefPtr<Gtk::TextBuffer> &buf);
void HandleRoleMentions(const Glib::RefPtr<Gtk::TextBuffer> &buf);
void HandleUserMentions(const Glib::RefPtr<Gtk::TextBuffer> &buf, Snowflake channel_id, bool plain);
void HandleStockEmojis(Gtk::TextView &tv);
void HandleCustomEmojis(Gtk::TextView &tv);
void HandleEmojis(Gtk::TextView &tv);
void CleanupEmojis(const Glib::RefPtr<Gtk::TextBuffer> &buf);
} // namespace ChatUtil
