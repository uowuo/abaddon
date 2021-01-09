#include "channels.hpp"
#include <algorithm>
#include <map>
#include <unordered_map>
#include "../abaddon.hpp"
#include "../imgmanager.hpp"
#include "../util.hpp"

void ChannelListRow::Collapse() {}

void ChannelListRow::Expand() {}

void ChannelListRow::MakeReadOnly(Gtk::TextView *tv) {
    tv->set_can_focus(false);
    tv->set_editable(false);
    tv->signal_realize().connect([tv]() {
        auto window = tv->get_window(Gtk::TEXT_WINDOW_TEXT);
        auto display = window->get_display();
        auto cursor = Gdk::Cursor::create(display, "default"); // textview uses "text" which looks out of place
        window->set_cursor(cursor);
    });
    // stupid hack to prevent selection
    auto buf = tv->get_buffer();
    buf->property_has_selection().signal_changed().connect([tv, buf]() {
        Gtk::TextBuffer::iterator a, b;
        buf->get_bounds(a, b);
        buf->select_range(a, a);
    });
}

ChannelListRowDMHeader::ChannelListRowDMHeader() {
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_lbl = Gtk::manage(new Gtk::Label);

    get_style_context()->add_class("channel-row");
    m_lbl->get_style_context()->add_class("channel-row-label");

    m_lbl->set_use_markup(true);
    m_lbl->set_markup("<b>Direct Messages</b>");
    m_box->set_halign(Gtk::ALIGN_START);
    m_box->pack_start(*m_lbl);

    m_ev->add(*m_box);
    add(*m_ev);
    show_all_children();
}

ChannelListRowDMChannel::ChannelListRowDMChannel(const ChannelData *data) {
    ID = data->ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_lbl = Gtk::manage(new Gtk::TextView);
    MakeReadOnly(m_lbl);

    get_style_context()->add_class("channel-row");
    m_lbl->get_style_context()->add_class("channel-row-label");

    UserData top_recipient;
    const auto recipients = data->GetDMRecipients();
    top_recipient = recipients[0];

    if (data->Type == ChannelType::DM) {
        if (top_recipient.HasAvatar()) {
            auto buf = Abaddon::Get().GetImageManager().GetFromURLIfCached(top_recipient.GetAvatarURL("png", "16"));
            if (buf)
                m_icon = Gtk::manage(new Gtk::Image(buf));
            else {
                m_icon = Gtk::manage(new Gtk::Image(Abaddon::Get().GetImageManager().GetPlaceholder(24)));
                Abaddon::Get().GetImageManager().LoadFromURL(top_recipient.GetAvatarURL("png", "16"), sigc::mem_fun(*this, &ChannelListRowDMChannel::OnImageLoad));
            }
        } else {
            m_icon = Gtk::manage(new Gtk::Image(Abaddon::Get().GetImageManager().GetPlaceholder(24)));
        }
    }

    auto buf = m_lbl->get_buffer();
    if (data->Type == ChannelType::DM)
        buf->set_text(top_recipient.Username);
    else if (data->Type == ChannelType::GROUP_DM)
        buf->set_text(std::to_string(recipients.size()) + " users");
    Abaddon::Get().GetEmojis().ReplaceEmojis(buf, ChannelEmojiSize);

    m_box->set_halign(Gtk::ALIGN_START);
    if (m_icon != nullptr)
        m_box->pack_start(*m_icon);
    m_box->pack_start(*m_lbl);
    m_ev->add(*m_box);
    add(*m_ev);
    show_all_children();
}

void ChannelListRowDMChannel::OnImageLoad(Glib::RefPtr<Gdk::Pixbuf> buf) {
    if (m_icon != nullptr)
        m_icon->property_pixbuf() = buf->scale_simple(24, 24, Gdk::INTERP_BILINEAR);
}

ChannelListRowGuild::ChannelListRowGuild(const GuildData *data) {
    ID = data->ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_lbl = Gtk::manage(new Gtk::TextView);
    MakeReadOnly(m_lbl);

    AddWidgetMenuHandler(m_ev, m_menu);
    AddWidgetMenuHandler(m_lbl, m_menu);

    m_menu_copyid = Gtk::manage(new Gtk::MenuItem("_Copy ID", true));
    m_menu_copyid->signal_activate().connect([this]() {
        m_signal_copy_id.emit();
    });
    m_menu.append(*m_menu_copyid);

    m_menu_leave = Gtk::manage(new Gtk::MenuItem("_Leave Guild", true));
    m_menu_leave->signal_activate().connect([this]() {
        m_signal_leave.emit();
    });
    m_menu.append(*m_menu_leave);

    m_menu.show_all();

    const auto show_animations = Abaddon::Get().GetSettings().GetShowAnimations();
    auto &img = Abaddon::Get().GetImageManager();
    if (data->HasIcon()) {
        if (data->HasAnimatedIcon() && show_animations) {
            auto buf = img.GetAnimationFromURLIfCached(data->GetIconURL("gif", "32"), 24, 24);
            if (buf)
                m_icon = Gtk::manage(new Gtk::Image(buf));
            else {
                m_icon = Gtk::manage(new Gtk::Image(img.GetPlaceholder(24)));
                img.LoadAnimationFromURL(data->GetIconURL("gif", "32"), 24, 24, sigc::mem_fun(*this, &ChannelListRowGuild::OnAnimatedImageLoad));
            }
        } else {
            auto buf = img.GetFromURLIfCached(data->GetIconURL("png", "32"));
            if (buf)
                m_icon = Gtk::manage(new Gtk::Image(buf->scale_simple(24, 24, Gdk::INTERP_BILINEAR)));
            else {
                m_icon = Gtk::manage(new Gtk::Image(img.GetPlaceholder(24)));
                img.LoadFromURL(data->GetIconURL("png", "32"), sigc::mem_fun(*this, &ChannelListRowGuild::OnImageLoad));
            }
        }
    } else {
        m_icon = Gtk::manage(new Gtk::Image(Abaddon::Get().GetImageManager().GetPlaceholder(24)));
    }

    get_style_context()->add_class("channel-row");
    get_style_context()->add_class("channel-row-guild");
    m_lbl->get_style_context()->add_class("channel-row-label");

    auto buf = m_lbl->get_buffer();
    Gtk::TextBuffer::iterator start, end;
    buf->get_bounds(start, end);
    buf->insert_markup(start, "<b>" + Glib::Markup::escape_text(data->Name) + "</b>");
    Abaddon::Get().GetEmojis().ReplaceEmojis(buf, ChannelEmojiSize);
    m_box->set_halign(Gtk::ALIGN_START);
    m_box->pack_start(*m_icon);
    m_box->pack_start(*m_lbl);
    m_ev->add(*m_box);
    add(*m_ev);
    show_all_children();
}

void ChannelListRowGuild::OnImageLoad(const Glib::RefPtr<Gdk::Pixbuf> &buf) {
    m_icon->property_pixbuf() = buf->scale_simple(24, 24, Gdk::INTERP_BILINEAR);
}

void ChannelListRowGuild::OnAnimatedImageLoad(const Glib::RefPtr<Gdk::PixbufAnimation> &buf) {
    m_icon->property_pixbuf_animation() = buf;
}

ChannelListRowGuild::type_signal_copy_id ChannelListRowGuild::signal_copy_id() {
    return m_signal_copy_id;
}

ChannelListRowGuild::type_signal_leave ChannelListRowGuild::signal_leave() {
    return m_signal_leave;
}

ChannelListRowCategory::ChannelListRowCategory(const ChannelData *data) {
    ID = data->ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_lbl = Gtk::manage(new Gtk::TextView);
    MakeReadOnly(m_lbl);
    m_arrow = Gtk::manage(new Gtk::Arrow(Gtk::ARROW_DOWN, Gtk::SHADOW_NONE));

    m_menu_copyid = Gtk::manage(new Gtk::MenuItem("_Copy ID", true));
    m_menu_copyid->signal_activate().connect([this]() {
        m_signal_copy_id.emit();
    });
    m_menu.append(*m_menu_copyid);

    m_menu.show_all();

    AddWidgetMenuHandler(m_ev, m_menu);
    AddWidgetMenuHandler(m_lbl, m_menu);

    get_style_context()->add_class("channel-row");
    get_style_context()->add_class("channel-row-category");
    m_lbl->get_style_context()->add_class("channel-row-label");

    auto buf = m_lbl->get_buffer();
    buf->set_text(*data->Name);
    Abaddon::Get().GetEmojis().ReplaceEmojis(buf, ChannelEmojiSize);
    m_box->set_halign(Gtk::ALIGN_START);
    m_box->pack_start(*m_arrow);
    m_box->pack_start(*m_lbl);
    m_ev->add(*m_box);
    add(*m_ev);
    show_all_children();
}

void ChannelListRowCategory::Collapse() {
    m_arrow->set(Gtk::ARROW_RIGHT, Gtk::SHADOW_NONE);
}

void ChannelListRowCategory::Expand() {
    m_arrow->set(IsUserCollapsed ? Gtk::ARROW_RIGHT : Gtk::ARROW_DOWN, Gtk::SHADOW_NONE);
}

ChannelListRowCategory::type_signal_copy_id ChannelListRowCategory::signal_copy_id() {
    return m_signal_copy_id;
}

ChannelListRowChannel::ChannelListRowChannel(const ChannelData *data) {
    ID = data->ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_lbl = Gtk::manage(new Gtk::TextView);
    MakeReadOnly(m_lbl);

    m_menu_copyid = Gtk::manage(new Gtk::MenuItem("_Copy ID", true));
    m_menu_copyid->signal_activate().connect([this]() {
        m_signal_copy_id.emit();
    });
    m_menu.append(*m_menu_copyid);

    m_menu.show_all();

    AddWidgetMenuHandler(m_ev, m_menu);
    AddWidgetMenuHandler(m_lbl, m_menu);

    get_style_context()->add_class("channel-row");
    get_style_context()->add_class("channel-row-channel");
    m_lbl->get_style_context()->add_class("channel-row-label");

    auto buf = m_lbl->get_buffer();
    buf->set_text("#" + *data->Name);
    Abaddon::Get().GetEmojis().ReplaceEmojis(buf, ChannelEmojiSize);
    m_box->set_halign(Gtk::ALIGN_START);
    m_box->pack_start(*m_lbl);
    m_ev->add(*m_box);
    add(*m_ev);
    show_all_children();
}

ChannelListRowChannel::type_signal_copy_id ChannelListRowChannel::signal_copy_id() {
    return m_signal_copy_id;
}

ChannelList::ChannelList() {
    m_main = Gtk::manage(new Gtk::ScrolledWindow);
    m_list = Gtk::manage(new Gtk::ListBox);

    m_list->get_style_context()->add_class("channel-list");

    m_list->set_activate_on_single_click(true);
    m_list->signal_row_activated().connect(sigc::mem_fun(*this, &ChannelList::on_row_activated));

    m_main->add(*m_list);
    m_main->show_all();

    m_update_dispatcher.connect(sigc::mem_fun(*this, &ChannelList::UpdateListingInternal));

    // maybe will regret doing it this way
    auto &discord = Abaddon::Get().GetDiscordClient();
    discord.signal_message_create().connect(sigc::track_obj([this, &discord](Snowflake message_id) {
        const auto message = discord.GetMessage(message_id);
        const auto channel = discord.GetChannel(message->ChannelID);
        if (!channel.has_value()) return;
        if (channel->Type == ChannelType::DM || channel->Type == ChannelType::GROUP_DM)
            CheckBumpDM(message->ChannelID);
        // clang-format off
    }, this));
    // clang-format on
}

Gtk::Widget *ChannelList::GetRoot() const {
    return m_main;
}

void ChannelList::UpdateListing() {
    //std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_update_dispatcher.emit();
}

void ChannelList::UpdateNewGuild(Snowflake id) {
    auto sort = Abaddon::Get().GetDiscordClient().GetUserSortedGuilds();
    if (sort.size() == 1) {
        UpdateListing();
        return;
    }

    const auto insert_at = [this, id](int listpos) {
        InsertGuildAt(id, listpos);
    };

    auto it = std::find(sort.begin(), sort.end(), id);
    if (it == sort.end()) return;
    // if the new guild pos is at the end use -1
    if (it + 1 == sort.end()) {
        insert_at(-1);
        return;
    }
    // find the position of the guild below it into the listbox
    auto below_id = *(it + 1);
    auto below_it = m_id_to_row.find(below_id);
    if (below_it == m_id_to_row.end()) {
        UpdateListing();
        return;
    }
    auto below_pos = below_it->second->get_index();
    // stick it just above
    insert_at(below_pos - 1);
}

void ChannelList::UpdateRemoveGuild(Snowflake id) {
    auto it = m_guild_id_to_row.find(id);
    if (it == m_guild_id_to_row.end()) return;
    auto row = dynamic_cast<ChannelListRow *>(it->second);
    if (row == nullptr) return;
    DeleteRow(row);
}

void ChannelList::UpdateRemoveChannel(Snowflake id) {
    auto it = m_id_to_row.find(id);
    if (it == m_id_to_row.end()) return;
    auto row = dynamic_cast<ChannelListRow *>(it->second);
    if (row == nullptr) return;
    DeleteRow(row);
}

// this is total shit
void ChannelList::UpdateChannelCategory(Snowflake id) {
    const auto data = Abaddon::Get().GetDiscordClient().GetChannel(id);
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(*data->GuildID);
    auto git = m_guild_id_to_row.find(*data->GuildID);
    if (git == m_guild_id_to_row.end()) return;
    auto *guild_row = git->second;
    if (!data.has_value() || !guild.has_value()) return;
    auto it = m_id_to_row.find(id);
    if (it == m_id_to_row.end()) return;
    auto row = dynamic_cast<ChannelListRowCategory *>(it->second);
    if (row == nullptr) return;
    const bool old_collapsed = row->IsUserCollapsed;
    const bool visible = row->is_visible();
    std::map<int, Snowflake> child_rows;
    for (auto child : row->Children) {
        child_rows[child->get_index()] = child->ID;
    }
    guild_row->Children.erase(row);
    DeleteRow(row);

    int pos = guild_row->get_index();
    const auto sorted = guild->GetSortedChannels(id);
    const auto sorted_it = std::find(sorted.begin(), sorted.end(), id);
    if (sorted_it == sorted.end()) return;
    if (std::next(sorted_it) == sorted.end()) {
        const auto x = m_id_to_row.find(*std::prev(sorted_it));
        if (x != m_id_to_row.end())
            pos = x->second->get_index() + 1;
    } else {
        const auto x = m_id_to_row.find(*std::next(sorted_it));
        if (x != m_id_to_row.end())
            pos = x->second->get_index();
    }

    auto *new_row = Gtk::manage(new ChannelListRowCategory(&*data));
    new_row->IsUserCollapsed = old_collapsed;
    if (visible)
        new_row->show();
    m_id_to_row[id] = new_row;
    new_row->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), new_row->ID));
    new_row->Parent = guild_row;
    guild_row->Children.insert(new_row);
    m_list->insert(*new_row, pos);
    int i = 1;
    for (const auto &[idx, child_id] : child_rows) {
        const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(child_id);
        if (channel.has_value()) {
            auto *new_child = Gtk::manage(new ChannelListRowChannel(&*channel));
            new_row->Children.insert(new_child);
            new_child->Parent = new_row;
            new_child->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), new_child->ID));
            m_id_to_row[child_id] = new_child;
            if (visible && !new_row->IsUserCollapsed)
                new_child->show();
            m_list->insert(*new_child, pos + i++);
        }
    }
}

// so is this
void ChannelList::UpdateChannel(Snowflake id) {
    const auto data = Abaddon::Get().GetDiscordClient().GetChannel(id);
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(*data->GuildID);
    const auto *guild_row = m_guild_id_to_row.at(*data->GuildID);
    if (data->Type == ChannelType::GUILD_CATEGORY) {
        UpdateChannelCategory(id);
        return;
    }

    auto it = m_id_to_row.find(id);
    if (it == m_id_to_row.end()) return; // stuff like voice doesnt have a row yet
    auto row = dynamic_cast<ChannelListRowChannel *>(it->second);
    const bool old_collapsed = row->IsUserCollapsed;
    const bool old_visible = row->is_visible();
    DeleteRow(row);

    int pos = guild_row->get_index() + 1; // fallback
    const auto sorted = guild->GetSortedChannels();
    const auto sorted_it = std::find(sorted.begin(), sorted.end(), id);
    if (sorted_it + 1 == sorted.end()) {
        const auto x = m_id_to_row.find(*std::prev(sorted_it));
        if (x != m_id_to_row.end())
            pos = x->second->get_index() + 1;
    } else {
        const auto x = m_id_to_row.find(*std::next(sorted_it));
        if (x != m_id_to_row.end())
            pos = x->second->get_index();
    }

    auto *new_row = Gtk::manage(new ChannelListRowChannel(&*data));
    new_row->IsUserCollapsed = old_collapsed;
    m_id_to_row[id] = new_row;
    if (data->ParentID.has_value()) {
        new_row->Parent = m_id_to_row.at(*data->ParentID);
    } else {
        new_row->Parent = m_guild_id_to_row.at(*data->GuildID);
    }
    new_row->Parent->Children.insert(new_row);
    if (new_row->Parent->is_visible() && !new_row->Parent->IsUserCollapsed)
        new_row->show();
    new_row->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), new_row->ID));
    m_list->insert(*new_row, pos);
}

void ChannelList::UpdateCreateDMChannel(Snowflake id) {
    const auto chan = Abaddon::Get().GetDiscordClient().GetChannel(id);
    auto *dm_row = Gtk::manage(new ChannelListRowDMChannel(&*chan));
    dm_row->IsUserCollapsed = false;
    m_list->insert(*dm_row, m_dm_header_row->get_index() + 1);
    m_dm_header_row->Children.insert(dm_row);
    if (!m_dm_header_row->IsUserCollapsed)
        dm_row->show();
}

void ChannelList::UpdateCreateChannel(Snowflake id) {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto data = discord.GetChannel(id);
    if (data->Type == ChannelType::DM || data->Type == ChannelType::GROUP_DM) {
        UpdateCreateDMChannel(id);
        return;
    }
    const auto guild = discord.GetGuild(*data->GuildID);
    auto *guild_row = m_guild_id_to_row.at(*data->GuildID);

    int pos = guild_row->get_index() + 1;
    const auto sorted = guild->GetSortedChannels();
    const auto sorted_it = std::find(sorted.begin(), sorted.end(), id);
    if (sorted_it + 1 == sorted.end()) {
        const auto x = m_id_to_row.find(*std::prev(sorted_it));
        if (x != m_id_to_row.end())
            pos = x->second->get_index() + 1;
    } else {
        const auto x = m_id_to_row.find(*std::next(sorted_it));
        if (x != m_id_to_row.end())
            pos = x->second->get_index();
    }

    ChannelListRow *row;
    if (data->Type == ChannelType::GUILD_TEXT || data->Type == ChannelType::GUILD_NEWS) {
        auto *tmp = Gtk::manage(new ChannelListRowChannel(&*data));
        tmp->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), tmp->ID));
        row = tmp;
    } else if (data->Type == ChannelType::GUILD_CATEGORY) {
        auto *tmp = Gtk::manage(new ChannelListRowCategory(&*data));
        tmp->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), tmp->ID));
        row = tmp;
    } else
        return;
    row->IsUserCollapsed = false;
    if (guild_row->is_visible())
        row->show();
    row->Parent = guild_row;
    guild_row->Children.insert(row);
    m_id_to_row[id] = row;
    m_list->insert(*row, pos);
}

void ChannelList::UpdateGuild(Snowflake id) {
    // the only thing changed is the row containing the guild item so just recreate it
    const auto data = Abaddon::Get().GetDiscordClient().GetGuild(id);
    if (!data.has_value()) return;
    auto it = m_guild_id_to_row.find(id);
    if (it == m_guild_id_to_row.end()) return;
    auto *row = dynamic_cast<ChannelListRowGuild *>(it->second);
    const auto children = row->Children;
    const auto index = row->get_index();
    const bool old_collapsed = row->IsUserCollapsed;
    const bool old_gindex = row->GuildIndex;
    delete row;
    auto *new_row = Gtk::manage(new ChannelListRowGuild(&*data));
    new_row->IsUserCollapsed = old_collapsed;
    new_row->GuildIndex = old_gindex;
    m_guild_id_to_row[new_row->ID] = new_row;
    new_row->signal_leave().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnGuildMenuLeave), new_row->ID));
    new_row->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), new_row->ID));
    new_row->Children = children;
    for (auto child : children)
        child->Parent = new_row;
    new_row->show_all();
    m_list->insert(*new_row, index);
}

void ChannelList::Clear() {
    //std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_update_dispatcher.emit();
}

void ChannelList::SetActiveChannel(Snowflake id) {
    auto it = m_id_to_row.find(id);
    if (it == m_id_to_row.end()) return;
    m_list->select_row(*it->second);
}

void ChannelList::CollapseRow(ChannelListRow *row) {
    row->Collapse();
    for (auto child : row->Children) {
        child->hide();
        CollapseRow(child);
    }
}

void ChannelList::ExpandRow(ChannelListRow *row) {
    row->Expand();
    row->show();
    if (!row->IsUserCollapsed)
        for (auto child : row->Children)
            ExpandRow(child);
}

void ChannelList::DeleteRow(ChannelListRow *row) {
    for (auto child : row->Children)
        DeleteRow(child);
    if (row->Parent != nullptr)
        row->Parent->Children.erase(row);
    else
        printf("row has no parent!\n");
    if (dynamic_cast<ChannelListRowGuild *>(row) != nullptr)
        m_guild_id_to_row.erase(row->ID);
    else
        m_id_to_row.erase(row->ID);
    delete row;
}

void ChannelList::on_row_activated(Gtk::ListBoxRow *tmprow) {
    auto row = dynamic_cast<ChannelListRow *>(tmprow);
    if (row == nullptr) return;
    bool new_collapsed = !row->IsUserCollapsed;
    row->IsUserCollapsed = new_collapsed;

    // kinda ugly
    if (dynamic_cast<ChannelListRowChannel *>(row) != nullptr || dynamic_cast<ChannelListRowDMChannel *>(row) != nullptr)
        m_signal_action_channel_item_select.emit(row->ID);

    if (new_collapsed)
        CollapseRow(row);
    else
        ExpandRow(row);
}

void ChannelList::InsertGuildAt(Snowflake id, int pos) {
    const auto insert_and_adjust = [&](Gtk::Widget &widget) {
        m_list->insert(widget, pos);
        if (pos != -1) pos++;
    };

    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto guild_data = discord.GetGuild(id);
    if (!guild_data.has_value()) return;

    std::map<int, ChannelData> orphan_channels;
    std::unordered_map<Snowflake, std::vector<ChannelData>> cat_to_channels;
    if (guild_data->Channels.has_value())
        for (const auto &dc : *guild_data->Channels) {
            const auto channel = discord.GetChannel(dc.ID);
            if (!channel.has_value()) continue;
            if (channel->Type != ChannelType::GUILD_TEXT && channel->Type != ChannelType::GUILD_NEWS) continue;

            if (channel->ParentID.has_value())
                cat_to_channels[*channel->ParentID].push_back(*channel);
            else
                orphan_channels[*channel->Position] = *channel;
        }

    auto *guild_row = Gtk::manage(new ChannelListRowGuild(&*guild_data));
    guild_row->show_all();
    guild_row->IsUserCollapsed = true;
    guild_row->GuildIndex = m_guild_count++;
    insert_and_adjust(*guild_row);
    m_guild_id_to_row[guild_row->ID] = guild_row;
    guild_row->signal_leave().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnGuildMenuLeave), guild_row->ID));
    guild_row->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), guild_row->ID));

    // add channels with no parent category
    for (const auto &[pos, channel] : orphan_channels) {
        auto *chan_row = Gtk::manage(new ChannelListRowChannel(&channel));
        chan_row->IsUserCollapsed = false;
        chan_row->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), chan_row->ID));
        insert_and_adjust(*chan_row);
        guild_row->Children.insert(chan_row);
        chan_row->Parent = guild_row;
        m_id_to_row[chan_row->ID] = chan_row;
    }

    // categories
    std::map<int, std::vector<ChannelData>> sorted_categories;
    if (guild_data->Channels.has_value())
        for (const auto &dc : *guild_data->Channels) {
            const auto channel = discord.GetChannel(dc.ID);
            if (!channel.has_value()) continue;
            if (channel->Type == ChannelType::GUILD_CATEGORY)
                sorted_categories[*channel->Position].push_back(*channel);
        }

    for (auto &[pos, catvec] : sorted_categories) {
        std::sort(catvec.begin(), catvec.end(), [](const ChannelData &a, const ChannelData &b) { return a.ID < b.ID; });
        for (const auto cat : catvec) {
            auto *cat_row = Gtk::manage(new ChannelListRowCategory(&cat));
            cat_row->IsUserCollapsed = false;
            cat_row->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), cat_row->ID));
            insert_and_adjust(*cat_row);
            guild_row->Children.insert(cat_row);
            cat_row->Parent = guild_row;
            m_id_to_row[cat_row->ID] = cat_row;

            // child channels
            if (cat_to_channels.find(cat.ID) == cat_to_channels.end()) continue;
            std::map<int, ChannelData> sorted_channels;

            for (const auto channel : cat_to_channels.at(cat.ID))
                sorted_channels[*channel.Position] = channel;

            for (const auto &[pos, channel] : sorted_channels) {
                auto *chan_row = Gtk::manage(new ChannelListRowChannel(&channel));
                chan_row->IsUserCollapsed = false;
                chan_row->signal_copy_id().connect(sigc::bind(sigc::mem_fun(*this, &ChannelList::OnMenuCopyID), chan_row->ID));
                insert_and_adjust(*chan_row);
                cat_row->Children.insert(chan_row);
                chan_row->Parent = cat_row;
                m_id_to_row[chan_row->ID] = chan_row;
            }
        }
    }
}

void ChannelList::AddPrivateChannels() {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    auto dms_ = discord.GetPrivateChannels();
    std::vector<ChannelData> dms;
    for (const auto &x : dms_) {
        const auto chan = discord.GetChannel(x);
        dms.push_back(*chan);
    }
    std::sort(dms.begin(), dms.end(), [&](const ChannelData &a, const ChannelData &b) -> bool {
        return a.LastMessageID > b.LastMessageID;
    });

    m_dm_header_row = Gtk::manage(new ChannelListRowDMHeader);
    m_dm_header_row->show_all();
    m_dm_header_row->IsUserCollapsed = true;
    m_list->add(*m_dm_header_row);

    for (const auto &dm : dms) {
        auto *dm_row = Gtk::manage(new ChannelListRowDMChannel(&dm));
        dm_row->Parent = m_dm_header_row;
        m_dm_id_to_row[dm.ID] = dm_row;
        dm_row->IsUserCollapsed = false;
        m_list->add(*dm_row);
        m_dm_header_row->Children.insert(dm_row);
    }
}

void ChannelList::UpdateListingInternal() {
    std::unordered_set<Snowflake> guilds = Abaddon::Get().GetDiscordClient().GetGuilds();

    auto children = m_list->get_children();
    auto it = children.begin();

    while (it != children.end()) {
        delete *it;
        it++;
    }

    m_id_to_row.clear();

    m_guild_count = 0;

    AddPrivateChannels();

    auto sorted_guilds = Abaddon::Get().GetDiscordClient().GetUserSortedGuilds();
    for (auto gid : sorted_guilds) {
        InsertGuildAt(gid, -1);
    }
}

void ChannelList::OnMenuCopyID(Snowflake id) {
    Gtk::Clipboard::get()->set_text(std::to_string(id));
}

void ChannelList::OnGuildMenuLeave(Snowflake id) {
    m_signal_action_guild_leave.emit(id);
}

void ChannelList::CheckBumpDM(Snowflake channel_id) {
    auto it = m_dm_id_to_row.find(channel_id);
    if (it == m_dm_id_to_row.end()) return;
    auto *row = it->second;
    const auto index = row->get_index();
    if (index == 1) return; // 1 is top of dm list
    const bool selected = row->is_selected();
    row->Parent->Children.erase(row);
    delete row;
    const auto chan = Abaddon::Get().GetDiscordClient().GetChannel(channel_id);
    auto *dm_row = Gtk::manage(new ChannelListRowDMChannel(&*chan));
    dm_row->Parent = m_dm_header_row;
    m_dm_header_row->Children.insert(dm_row);
    m_dm_id_to_row[channel_id] = dm_row;
    dm_row->IsUserCollapsed = false;
    m_list->insert(*dm_row, 1);
    m_dm_header_row->Children.insert(dm_row);
    if (selected)
        m_list->select_row(*dm_row);
    if (m_dm_header_row->is_visible() && !m_dm_header_row->IsUserCollapsed)
        dm_row->show();
}

ChannelList::type_signal_action_channel_item_select ChannelList::signal_action_channel_item_select() {
    return m_signal_action_channel_item_select;
}

ChannelList::type_signal_action_guild_leave ChannelList::signal_action_guild_leave() {
    return m_signal_action_guild_leave;
}
