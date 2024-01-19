#include <unordered_set>
#include <utility>
#include "completer.hpp"
#include "abaddon.hpp"
#include "util.hpp"

constexpr const int CompleterHeight = 150;
constexpr const int MaxCompleterEntries = 30;

Completer::Completer() {
    set_reveal_child(false);
    set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_NONE); // only SLIDE_UP and NONE work decently

    m_scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    m_scroll.set_max_content_height(CompleterHeight);
    m_scroll.set_size_request(-1, CompleterHeight);
    m_scroll.set_placement(Gtk::CORNER_BOTTOM_LEFT);

    m_list.set_adjustment(m_scroll.get_vadjustment());
    m_list.set_focus_vadjustment(m_scroll.get_vadjustment());
    m_list.get_style_context()->add_class("completer");
    m_list.set_activate_on_single_click(true);

    m_list.set_focus_on_click(false);
    set_can_focus(false);

    m_list.signal_row_activated().connect(sigc::mem_fun(*this, &Completer::OnRowActivate));

    m_scroll.add(m_list);
    add(m_scroll);
    show_all();
}

Completer::Completer(const Glib::RefPtr<Gtk::TextBuffer> &buf)
    : Completer() {
    SetBuffer(buf);
}

void Completer::SetBuffer(const Glib::RefPtr<Gtk::TextBuffer> &buf) {
    m_buf = buf;
    m_buf->signal_changed().connect(sigc::mem_fun(*this, &Completer::OnTextBufferChanged));
}

bool Completer::ProcessKeyPress(GdkEventKey *e) {
    if (!IsShown()) return false;
    if (e->type != GDK_KEY_PRESS) return false;

    switch (e->keyval) {
        case GDK_KEY_Down: {
            if (m_entries.empty()) return true;
            const auto index = static_cast<size_t>(m_list.get_selected_row()->get_index());
            if (index >= m_entries.size() - 1) return true;
            m_list.select_row(*m_entries[index + 1]);
            ScrollListBoxToSelected(m_list);
        }
            return true;
        case GDK_KEY_Up: {
            if (m_entries.empty()) return true;
            const auto index = static_cast<size_t>(m_list.get_selected_row()->get_index());
            if (index == 0) return true;
            m_list.select_row(*m_entries[index - 1]);
            ScrollListBoxToSelected(m_list);
        }
            return true;
        case GDK_KEY_Return: {
            if (m_entries.empty()) return true;
            DoCompletion(m_list.get_selected_row());
        }
            return true;
        default:
            break;
    }

    return false;
}

void Completer::SetGetRecentAuthors(get_recent_authors_cb cb) {
    m_recent_authors_cb = std::move(cb);
}

void Completer::SetGetChannelID(get_channel_id_cb cb) {
    m_channel_id_cb = std::move(cb);
}

bool Completer::IsShown() const {
    return get_child_revealed();
}

CompleterEntry *Completer::CreateEntry(const Glib::ustring &completion) {
    auto entry = Gtk::manage(new CompleterEntry(completion, static_cast<int>(m_entries.size())));
    m_entries.push_back(entry);
    entry->show_all();
    m_list.add(*entry);
    return entry;
}

void Completer::CompleteMentions(const Glib::ustring &term) {
    if (!m_recent_authors_cb)
        return;

    const auto &discord = Abaddon::Get().GetDiscordClient();

    Snowflake channel_id;
    if (m_channel_id_cb)
        channel_id = m_channel_id_cb();
    auto author_ids = m_recent_authors_cb();
    if (channel_id.IsValid()) {
        const auto chan = discord.GetChannel(channel_id);
        if (chan->GuildID.has_value()) {
            const auto members = discord.GetUsersInGuild(*chan->GuildID);
            for (const auto x : members)
                if (std::find(author_ids.begin(), author_ids.end(), x) == author_ids.end())
                    author_ids.push_back(x);
        }
    }
    const auto me = discord.GetUserData().ID;
    int i = 0;
    for (const auto id : author_ids) {
        if (id == me) continue;
        const auto author = discord.GetUser(id);
        if (!author.has_value()) continue;
        // todo improve the predicate here
        if (!StringContainsCaseless(author->Username, term)) continue;
        if (i++ > 15) break;

        auto entry = CreateEntry(author->GetMention());

        entry->SetText(author->GetUsername());

        if (channel_id.IsValid()) {
            const auto chan = discord.GetChannel(channel_id);
            if (chan.has_value() && chan->GuildID.has_value()) {
                const auto role_id = discord.GetMemberHoistedRole(*chan->GuildID, id, true);
                if (role_id.IsValid()) {
                    const auto role = discord.GetRole(role_id);
                    if (role.has_value())
                        entry->SetTextColor(role->Color);
                }
            }
        }

        entry->SetImage(author->GetAvatarURL());
    }
}

void Completer::CompleteEmojis(const Glib::ustring &term) {
    if (!m_channel_id_cb)
        return;

    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto channel_id = m_channel_id_cb();
    const auto channel = discord.GetChannel(channel_id);

    const auto make_entry = [&](const Glib::ustring &name, const Glib::ustring &completion, const Glib::ustring &url = "", bool animated = false) -> CompleterEntry * {
        const auto entry = CreateEntry(completion);
        entry->SetText(name);
        if (url.empty()) return entry;
        if (animated)
            entry->SetAnimation(url);
        else
            entry->SetImage(url);
        return entry;
    };

    const auto self_id = discord.GetUserData().ID;
    const bool can_use_external = discord.GetSelfPremiumType() != EPremiumType::None && discord.HasChannelPermission(self_id, channel_id, Permission::USE_EXTERNAL_EMOJIS);

    int i = 0;
    if (!can_use_external) {
        if (channel->GuildID.has_value()) {
            const auto guild = discord.GetGuild(*channel->GuildID);

            if (guild.has_value() && guild->Emojis.has_value())
                for (const auto &tmp : *guild->Emojis) {
                    const auto emoji = *discord.GetEmoji(tmp.ID);
                    if (emoji.IsAnimated.has_value() && *emoji.IsAnimated) continue;
                    if (emoji.IsAvailable.has_value() && !*emoji.IsAvailable) continue;
                    if (emoji.Roles.has_value() && !emoji.Roles->empty()) continue;
                    if (!term.empty())
                        if (!StringContainsCaseless(emoji.Name, term)) continue;

                    if (i++ > MaxCompleterEntries) break;

                    make_entry(emoji.Name, "<:" + emoji.Name + ":" + std::to_string(emoji.ID) + ">", emoji.GetURL());
                }
        }
    } else {
        for (const auto guild_id : discord.GetGuilds()) {
            const auto guild = discord.GetGuild(guild_id);
            if (!guild.has_value()) continue;
            for (const auto &tmp : *guild->Emojis) {
                const auto emoji = *discord.GetEmoji(tmp.ID);
                const bool is_animated = emoji.IsAnimated.has_value() && *emoji.IsAnimated;
                if (emoji.IsAvailable.has_value() && !*emoji.IsAvailable) continue;
                if (emoji.Roles.has_value() && !emoji.Roles->empty()) continue;
                if (!term.empty())
                    if (!StringContainsCaseless(emoji.Name, term)) continue;

                if (i++ > MaxCompleterEntries) goto done;

                if (is_animated)
                    make_entry(emoji.Name, "<a:" + emoji.Name + ":" + std::to_string(emoji.ID) + ">", emoji.GetURL("gif"), true);
                else
                    make_entry(emoji.Name, "<:" + emoji.Name + ":" + std::to_string(emoji.ID) + ">", emoji.GetURL());
            }
        }
    }
done:

    // if <15 guild emojis match then load up stock
    if (i < 15) {
        std::unordered_set<std::string> added_patterns;
        auto &emojis = Abaddon::Get().GetEmojis();
        const auto &shortcodes = emojis.GetShortCodes();
        for (const auto &[shortcode, pattern] : shortcodes) {
            if (added_patterns.find(pattern) != added_patterns.end()) continue;
            if (!StringContainsCaseless(shortcode, term)) continue;
            if (i++ > 15) break;
            const auto &pb = emojis.GetPixBuf(pattern);
            if (!pb) continue;
            added_patterns.insert(pattern);
            const auto entry = make_entry(shortcode, pattern);
            entry->SetImage(pb->scale_simple(CompleterImageSize, CompleterImageSize, Gdk::INTERP_BILINEAR));
        }
    }
}

void Completer::CompleteChannels(const Glib::ustring &term) {
    if (!m_channel_id_cb)
        return;

    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto channel_id = m_channel_id_cb();
    const auto channel = discord.GetChannel(channel_id);
    if (!channel->GuildID.has_value()) return;
    const auto channels = discord.GetChannelsInGuild(*channel->GuildID);
    int i = 0;
    for (const auto chan_id : channels) {
        const auto chan = discord.GetChannel(chan_id);
        if (chan->Type == ChannelType::GUILD_VOICE || chan->Type == ChannelType::GUILD_CATEGORY) continue;
        if (!StringContainsCaseless(*chan->Name, term)) continue;
        if (i++ > MaxCompleterEntries) break;
        const auto entry = CreateEntry("<#" + std::to_string(chan_id) + ">");
        entry->SetText("#" + *chan->Name);
    }
}

void Completer::DoCompletion(Gtk::ListBoxRow *row) {
    const int index = row->get_index();
    const auto completion = m_entries[index]->GetCompletion();
    const auto it = m_buf->erase(m_start, m_end); // entry is deleted here
    m_buf->insert(it, completion + " ");
}

void Completer::OnRowActivate(Gtk::ListBoxRow *row) {
    DoCompletion(row);
}

void Completer::OnTextBufferChanged() {
    const auto term = GetTerm();

    for (auto it = m_entries.begin(); it != m_entries.end();) {
        delete *it;
        it = m_entries.erase(it);
    }

    switch (term[0]) {
        case '@':
            CompleteMentions(term.substr(1));
            break;
        case ':':
            CompleteEmojis(term.substr(1));
            break;
        case '#':
            CompleteChannels(term.substr(1));
            break;
        default:
            break;
    }
    if (!m_entries.empty()) {
        m_list.select_row(*m_entries[0]);
        set_reveal_child(true);
    } else {
        set_reveal_child(false);
    }
}

bool MultiBackwardSearch(const Gtk::TextIter &iter, const Glib::ustring &chars, Gtk::TextSearchFlags flags, Gtk::TextBuffer::iterator &out) {
    bool any = false;
    for (const auto c : chars) {
        Glib::ustring tmp(1, c);
        Gtk::TextBuffer::iterator tstart, tend;
        if (!iter.backward_search(tmp, flags, tstart, tend)) continue;
        // if previous found, compare to see if closer to out iter
        if (any) {
            if (tstart.get_offset() > out.get_offset())
                out = tstart;
        } else
            out = tstart;
        any = true;
    }
    return any;
}

bool MultiForwardSearch(const Gtk::TextIter &iter, const Glib::ustring &chars, Gtk::TextSearchFlags flags, Gtk::TextBuffer::iterator &out) {
    bool any = false;
    for (const auto c : chars) {
        Glib::ustring tmp(1, c);
        Gtk::TextBuffer::iterator tstart, tend;
        if (!iter.forward_search(tmp, flags, tstart, tend)) continue;
        // if previous found, compare to see if closer to out iter
        if (any) {
            if (tstart.get_offset() < out.get_offset())
                out = tstart;
        } else
            out = tstart;
        any = true;
    }
    return any;
}

Glib::ustring Completer::GetTerm() {
    const auto iter = m_buf->get_insert()->get_iter();
    Gtk::TextBuffer::iterator dummy;
    if (!MultiBackwardSearch(iter, " \n", Gtk::TEXT_SEARCH_TEXT_ONLY, m_start))
        m_buf->get_bounds(m_start, dummy);
    else
        m_start.forward_char(); // 1 behind
    if (!MultiForwardSearch(iter, " \n", Gtk::TEXT_SEARCH_TEXT_ONLY, m_end))
        m_buf->get_bounds(dummy, m_end);
    return m_start.get_text(m_end);
}

CompleterEntry::CompleterEntry(Glib::ustring completion, int index)
    : m_completion(std::move(completion))
    , m_index(index)
    , m_box(Gtk::ORIENTATION_HORIZONTAL) {
    set_halign(Gtk::ALIGN_START);
    get_style_context()->add_class("completer-entry");
    set_can_focus(false);
    set_focus_on_click(false);
    m_box.show();
    add(m_box);
}

void CompleterEntry::SetTextColor(int color) {
    if (m_text == nullptr) return;
    const auto cur = m_text->get_text();
    m_text->set_markup("<span color='#" + IntToCSSColor(color) + "'>" + Glib::Markup::escape_text(cur) + "</span>");
}

void CompleterEntry::SetText(const Glib::ustring &text) {
    if (m_text == nullptr) {
        m_text = Gtk::manage(new Gtk::Label);
        m_text->get_style_context()->add_class("completer-entry-label");
        m_text->show();
        m_box.pack_end(*m_text);
    }
    m_text->set_label(text);
}

void CompleterEntry::SetImage(const Glib::RefPtr<Gdk::Pixbuf> &pb) {
    CheckImage();
    m_img->property_pixbuf() = pb;
}

void CompleterEntry::SetImage(const std::string &url) {
    CheckImage();
    m_img->SetAnimated(false);
    m_img->SetURL(url);
}

void CompleterEntry::SetAnimation(const std::string &url) {
    CheckImage();
    m_img->SetAnimated(true);
    m_img->SetURL(url);
}

void CompleterEntry::CheckImage() {
    if (m_img == nullptr) {
        m_img = Gtk::manage(new LazyImage(CompleterImageSize, CompleterImageSize));
        m_img->get_style_context()->add_class("completer-entry-image");
        m_img->show();
        m_box.pack_start(*m_img);
    }
}

int CompleterEntry::GetIndex() const {
    return m_index;
}

Glib::ustring CompleterEntry::GetCompletion() const {
    return m_completion;
}
