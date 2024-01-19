#include "chatutil.hpp"

#include "abaddon.hpp"
#include "constants.hpp"
#include "util.hpp"

namespace ChatUtil {
Glib::ustring GetText(const Glib::RefPtr<Gtk::TextBuffer> &buf) {
    Gtk::TextBuffer::iterator a, b;
    buf->get_bounds(a, b);
    auto slice = buf->get_slice(a, b, true);
    return slice;
}

void HandleRoleMentions(const Glib::RefPtr<Gtk::TextBuffer> &buf) {
    constexpr static const auto mentions_regex = R"(<@&(\d+)>)";

    static auto rgx = Glib::Regex::create(mentions_regex);

    Glib::ustring text = GetText(buf);
    const auto &discord = Abaddon::Get().GetDiscordClient();

    int startpos = 0;
    Glib::MatchInfo match;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        if (!match.fetch_pos(0, mstart, mend)) break;
        const Glib::ustring role_id = match.fetch(1);
        const auto role = discord.GetRole(role_id);
        if (!role.has_value()) {
            startpos = mend;
            continue;
        }

        Glib::ustring replacement;
        if (role->HasColor()) {
            replacement = "<b><span color=\"#" + IntToCSSColor(role->Color) + "\">@" + role->GetEscapedName() + "</span></b>";
        } else {
            replacement = "<b>@" + role->GetEscapedName() + "</b>";
        }

        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        const auto start_it = buf->get_iter_at_offset(chars_start);
        const auto end_it = buf->get_iter_at_offset(chars_end);

        auto it = buf->erase(start_it, end_it);
        buf->insert_markup(it, replacement);

        text = GetText(buf);
        startpos = 0;
    }
}

void HandleUserMentions(const Glib::RefPtr<Gtk::TextBuffer> &buf, Snowflake channel_id, bool plain) {
    constexpr static const auto mentions_regex = R"(<@!?(\d+)>)";

    static auto rgx = Glib::Regex::create(mentions_regex);

    Glib::ustring text = GetText(buf);
    const auto &discord = Abaddon::Get().GetDiscordClient();

    int startpos = 0;
    Glib::MatchInfo match;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        if (!match.fetch_pos(0, mstart, mend)) break;
        const Glib::ustring user_id = match.fetch(1);
        const auto user = discord.GetUser(user_id);
        const auto channel = discord.GetChannel(channel_id);
        if (!user.has_value() || !channel.has_value()) {
            startpos = mend;
            continue;
        }

        Glib::ustring replacement;

        if (channel->Type == ChannelType::DM || channel->Type == ChannelType::GROUP_DM || !channel->GuildID.has_value() || plain) {
            if (plain) {
                replacement = "@" + user->GetUsername();
            } else {
                replacement = user->GetUsernameEscapedBoldAt();
            }
        } else {
            const auto role_id = user->GetHoistedRole(*channel->GuildID, true);
            const auto role = discord.GetRole(role_id);
            if (!role.has_value())
                replacement = user->GetUsernameEscapedBoldAt();
            else
                replacement = "<span color=\"#" + IntToCSSColor(role->Color) + "\">" + user->GetUsernameEscapedBoldAt() + "</span>";
        }

        // regex returns byte positions and theres no straightforward way in the c++ bindings to deal with that :(
        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        const auto start_it = buf->get_iter_at_offset(chars_start);
        const auto end_it = buf->get_iter_at_offset(chars_end);

        auto it = buf->erase(start_it, end_it);
        buf->insert_markup(it, replacement);

        text = GetText(buf);
        startpos = 0;
    }
}

void HandleStockEmojis(Gtk::TextView &tv) {
    Abaddon::Get().GetEmojis().ReplaceEmojis(tv.get_buffer(), EmojiSize);
}

void HandleCustomEmojis(Gtk::TextView &tv) {
    static auto rgx = Glib::Regex::create(R"(<a?:([\w\d_]+):(\d+)>)");

    auto &img = Abaddon::Get().GetImageManager();

    auto buf = tv.get_buffer();
    auto text = GetText(buf);

    Glib::MatchInfo match;
    int startpos = 0;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        if (!match.fetch_pos(0, mstart, mend)) break;
        const bool is_animated = match.fetch(0)[1] == 'a';

        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        auto start_it = buf->get_iter_at_offset(chars_start);
        auto end_it = buf->get_iter_at_offset(chars_end);

        startpos = mend;
        if (is_animated && Abaddon::Get().GetSettings().ShowAnimations) {
            const auto mark_start = buf->create_mark(start_it, false);
            end_it.backward_char();
            const auto mark_end = buf->create_mark(end_it, false);
            const auto cb = [&tv, buf, mark_start, mark_end](const Glib::RefPtr<Gdk::PixbufAnimation> &pixbuf) {
                auto start_it = mark_start->get_iter();
                auto end_it = mark_end->get_iter();
                end_it.forward_char();
                buf->delete_mark(mark_start);
                buf->delete_mark(mark_end);
                auto it = buf->erase(start_it, end_it);
                const auto anchor = buf->create_child_anchor(it);
                auto img = Gtk::manage(new Gtk::Image(pixbuf));
                img->show();
                tv.add_child_at_anchor(*img, anchor);
            };
            img.LoadAnimationFromURL(EmojiData::URLFromID(match.fetch(2), "gif"), EmojiSize, EmojiSize, sigc::track_obj(cb, tv));
        } else {
            // can't erase before pixbuf is ready or else marks that are in the same pos get mixed up
            const auto mark_start = buf->create_mark(start_it, false);
            end_it.backward_char();
            const auto mark_end = buf->create_mark(end_it, false);
            const auto cb = [buf, mark_start, mark_end](const Glib::RefPtr<Gdk::Pixbuf> &pixbuf) {
                auto start_it = mark_start->get_iter();
                auto end_it = mark_end->get_iter();
                end_it.forward_char();
                buf->delete_mark(mark_start);
                buf->delete_mark(mark_end);
                auto it = buf->erase(start_it, end_it);
                int width, height;
                GetImageDimensions(pixbuf->get_width(), pixbuf->get_height(), width, height, EmojiSize, EmojiSize);
                buf->insert_pixbuf(it, pixbuf->scale_simple(width, height, Gdk::INTERP_BILINEAR));
            };
            img.LoadFromURL(EmojiData::URLFromID(match.fetch(2)), sigc::track_obj(cb, tv));
        }

        text = GetText(buf);
    }
}

void HandleEmojis(Gtk::TextView &tv) {
    if (Abaddon::Get().GetSettings().ShowStockEmojis) HandleStockEmojis(tv);
    if (Abaddon::Get().GetSettings().ShowCustomEmojis) HandleCustomEmojis(tv);
}

void CleanupEmojis(const Glib::RefPtr<Gtk::TextBuffer> &buf) {
    static auto rgx = Glib::Regex::create(R"(<a?:([\w\d_]+):(\d+)>)");

    auto text = GetText(buf);

    Glib::MatchInfo match;
    int startpos = 0;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        if (!match.fetch_pos(0, mstart, mend)) break;

        const auto new_term = ":" + match.fetch(1) + ":";

        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        auto start_it = buf->get_iter_at_offset(chars_start);
        auto end_it = buf->get_iter_at_offset(chars_end);

        startpos = mend;
        const auto it = buf->erase(start_it, end_it);
        const int alen = static_cast<int>(text.size());
        text = GetText(buf);
        const int blen = static_cast<int>(text.size());
        startpos -= (alen - blen);

        buf->insert(it, new_term);

        text = GetText(buf);
    }
}
} // namespace ChatUtil
