#include "channels.hpp"
#include <algorithm>
#include <map>
#include <unordered_map>
#include "../abaddon.hpp"
#include "../imgmanager.hpp"

void ChannelListRow::Collapse() {}

void ChannelListRow::Expand() {}

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

ChannelListRowDMChannel::ChannelListRowDMChannel(const Channel *data) {
    ID = data->ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_lbl = Gtk::manage(new Gtk::Label);

    get_style_context()->add_class("channel-row");
    m_lbl->get_style_context()->add_class("channel-row-label");

    if (data->Type == ChannelType::DM) {
        if (data->Recipients[0].HasAvatar()) {
            auto buf = Abaddon::Get().GetImageManager().GetFromURLIfCached(data->Recipients[0].GetAvatarURL("png", "16"));
            if (buf)
                m_icon = Gtk::manage(new Gtk::Image(buf));
            else {
                m_icon = Gtk::manage(new Gtk::Image(Abaddon::Get().GetImageManager().GetPlaceholder(24)));
                Abaddon::Get().GetImageManager().LoadFromURL(data->Recipients[0].GetAvatarURL("png", "16"), sigc::mem_fun(*this, &ChannelListRowDMChannel::OnImageLoad));
            }
        } else {
            m_icon = Gtk::manage(new Gtk::Image(Abaddon::Get().GetImageManager().GetPlaceholder(24)));
        }
    }

    if (data->Type == ChannelType::DM)
        m_lbl->set_text(data->Recipients[0].Username);
    else if (data->Type == ChannelType::GROUP_DM)
        m_lbl->set_text(std::to_string(data->Recipients.size()) + " users");

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

ChannelListRowGuild::ChannelListRowGuild(const Guild *data) {
    ID = data->ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_lbl = Gtk::manage(new Gtk::Label);

    if (data->HasIcon()) {
        auto buf = Abaddon::Get().GetImageManager().GetFromURLIfCached(data->GetIconURL("png", "32"));
        if (buf)
            m_icon = Gtk::manage(new Gtk::Image(buf->scale_simple(24, 24, Gdk::INTERP_BILINEAR)));
        else {
            m_icon = Gtk::manage(new Gtk::Image(Abaddon::Get().GetImageManager().GetPlaceholder(24)));
            Abaddon::Get().GetImageManager().LoadFromURL(data->GetIconURL("png", "32"), sigc::mem_fun(*this, &ChannelListRowGuild::OnImageLoad));
        }
    } else {
        m_icon = Gtk::manage(new Gtk::Image(Abaddon::Get().GetImageManager().GetPlaceholder(24)));
    }

    get_style_context()->add_class("channel-row");
    get_style_context()->add_class("channel-row-guild");
    m_lbl->get_style_context()->add_class("channel-row-label");

    m_lbl->set_markup("<b>" + Glib::Markup::escape_text(data->Name) + "</b>");
    m_box->set_halign(Gtk::ALIGN_START);
    m_box->pack_start(*m_icon);
    m_box->pack_start(*m_lbl);
    m_ev->add(*m_box);
    add(*m_ev);
    show_all_children();
}

void ChannelListRowGuild::OnImageLoad(Glib::RefPtr<Gdk::Pixbuf> buf) {
    m_icon->property_pixbuf() = buf->scale_simple(24, 24, Gdk::INTERP_BILINEAR);
}

ChannelListRowCategory::ChannelListRowCategory(const Channel *data) {
    ID = data->ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_lbl = Gtk::manage(new Gtk::Label);
    m_arrow = Gtk::manage(new Gtk::Arrow(Gtk::ARROW_DOWN, Gtk::SHADOW_NONE));

    get_style_context()->add_class("channel-row");
    get_style_context()->add_class("channel-row-category");
    m_lbl->get_style_context()->add_class("channel-row-label");

    m_lbl->set_text(data->Name);
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
    m_arrow->set(Gtk::ARROW_DOWN, Gtk::SHADOW_NONE);
}

ChannelListRowChannel::ChannelListRowChannel(const Channel *data) {
    ID = data->ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_lbl = Gtk::manage(new Gtk::Label);

    get_style_context()->add_class("channel-row");
    get_style_context()->add_class("channel-row-channel");
    m_lbl->get_style_context()->add_class("channel-row-label");

    m_lbl->set_text("#" + data->Name);
    m_box->set_halign(Gtk::ALIGN_START);
    m_box->pack_start(*m_lbl);
    m_ev->add(*m_box);
    add(*m_ev);
    show_all_children();
}

ChannelList::ChannelList() {
    m_main = Gtk::manage(new Gtk::ScrolledWindow);
    m_list = Gtk::manage(new Gtk::ListBox);

    m_list->get_style_context()->add_class("channel-list");

    m_guild_menu_up = Gtk::manage(new Gtk::MenuItem("Move _Up", true));
    m_guild_menu_up->signal_activate().connect(sigc::mem_fun(*this, &ChannelList::on_guild_menu_move_up));
    m_guild_menu.append(*m_guild_menu_up);

    m_guild_menu_down = Gtk::manage(new Gtk::MenuItem("Move _Down", true));
    m_guild_menu_down->signal_activate().connect(sigc::mem_fun(*this, &ChannelList::on_guild_menu_move_down));
    m_guild_menu.append(*m_guild_menu_down);

    m_guild_menu_copyid = Gtk::manage(new Gtk::MenuItem("_Copy ID", true));
    m_guild_menu_copyid->signal_activate().connect(sigc::mem_fun(*this, &ChannelList::on_guild_menu_copyid));
    m_guild_menu.append(*m_guild_menu_copyid);

    m_guild_menu_leave = Gtk::manage(new Gtk::MenuItem("_Leave Guild", true));
    m_guild_menu_leave->signal_activate().connect(sigc::mem_fun(*this, &ChannelList::on_guild_menu_leave));
    m_guild_menu.append(*m_guild_menu_leave);

    m_guild_menu.show_all();

    m_channel_menu_copyid = Gtk::manage(new Gtk::MenuItem("_Copy ID", true));
    m_channel_menu_copyid->signal_activate().connect(sigc::mem_fun(*this, &ChannelList::on_channel_menu_copyid));
    m_channel_menu.append(*m_channel_menu_copyid);

    m_channel_menu.show_all();

    m_list->set_activate_on_single_click(true);
    m_list->signal_row_activated().connect(sigc::mem_fun(*this, &ChannelList::on_row_activated));

    m_main->add(*m_list);
    m_main->show_all();

    m_update_dispatcher.connect(sigc::mem_fun(*this, &ChannelList::UpdateListingInternal));
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

void ChannelList::Clear() {
    //std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_update_dispatcher.emit();
}

void ChannelList::CollapseRow(ChannelListRow *row) {
    row->Collapse();
    for (auto child : row->Children) {
        row->IsHidden = true;
        child->hide();
        CollapseRow(child);
    }
}

void ChannelList::ExpandRow(ChannelListRow *row) {
    row->Expand();
    row->show();
    row->IsHidden = false;
    if (!row->IsUserCollapsed)
        for (auto child : row->Children)
            ExpandRow(child);
}

void ChannelList::DeleteRow(ChannelListRow *row) {
    for (auto child : row->Children)
        DeleteRow(child);
    if (row->Parent != nullptr)
        row->Parent->Children.erase(row);
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
    const auto *guild_data = discord.GetGuild(id);
    if (guild_data == nullptr) return;

    std::map<int, const Channel *> orphan_channels;
    std::unordered_map<Snowflake, std::vector<const Channel *>> cat_to_channels;
    for (const auto &channel : guild_data->Channels) {
        if (channel.Type != ChannelType::GUILD_TEXT) continue;

        if (channel.ParentID.IsValid())
            cat_to_channels[channel.ParentID].push_back(&channel);
        else
            orphan_channels[channel.Position] = &channel;
    }

    auto *guild_row = Gtk::manage(new ChannelListRowGuild(guild_data));
    guild_row->show_all();
    guild_row->IsUserCollapsed = true;
    guild_row->IsHidden = false;
    guild_row->GuildIndex = m_guild_count++;
    insert_and_adjust(*guild_row);
    m_guild_id_to_row[guild_row->ID] = guild_row;
    AttachGuildMenuHandler(guild_row);

    // add channels with no parent category
    for (const auto &[pos, channel] : orphan_channels) {
        auto *chan_row = Gtk::manage(new ChannelListRowChannel(channel));
        chan_row->IsUserCollapsed = false;
        chan_row->IsHidden = true;
        insert_and_adjust(*chan_row);
        guild_row->Children.insert(chan_row);
        chan_row->Parent = guild_row;
        m_id_to_row[chan_row->ID] = chan_row;
    }

    // categories
    std::map<int, std::vector<const Channel *>> sorted_categories;
    for (const auto &channel : guild_data->Channels)
        if (channel.Type == ChannelType::GUILD_CATEGORY)
            sorted_categories[channel.Position].push_back(&channel);

    for (auto &[pos, catvec] : sorted_categories) {
        std::sort(catvec.begin(), catvec.end(), [](const Channel *a, const Channel *b) { return a->ID < b->ID; });
        for (const auto cat : catvec) {
            auto *cat_row = Gtk::manage(new ChannelListRowCategory(cat));
            cat_row->IsUserCollapsed = false;
            cat_row->IsHidden = true;
            AttachChannelMenuHandler(cat_row);
            insert_and_adjust(*cat_row);
            guild_row->Children.insert(cat_row);
            cat_row->Parent = guild_row;
            m_id_to_row[cat_row->ID] = cat_row;

            // child channels
            if (cat_to_channels.find(cat->ID) == cat_to_channels.end()) continue;
            std::map<int, const Channel *> sorted_channels;

            for (const auto channel : cat_to_channels.at(cat->ID))
                sorted_channels[channel->Position] = channel;

            for (const auto &[pos, channel] : sorted_channels) {
                auto *chan_row = Gtk::manage(new ChannelListRowChannel(channel));
                chan_row->IsHidden = false;
                chan_row->IsUserCollapsed = false;
                AttachChannelMenuHandler(chan_row);
                insert_and_adjust(*chan_row);
                cat_row->Children.insert(chan_row);
                chan_row->Parent = cat_row;
                m_id_to_row[chan_row->ID] = chan_row;
            }
        }
    }
}

void ChannelList::AddPrivateChannels() {
    auto dms = Abaddon::Get().GetDiscordClient().GetPrivateChannels();

    auto *parent_row = Gtk::manage(new ChannelListRowDMHeader);
    parent_row->show_all();
    parent_row->IsUserCollapsed = true;
    parent_row->IsHidden = false;
    m_list->add(*parent_row);

    for (const auto &dm : dms) {
        auto *dm_row = Gtk::manage(new ChannelListRowDMChannel(Abaddon::Get().GetDiscordClient().GetChannel(dm)));
        dm_row->IsUserCollapsed = false;
        dm_row->IsHidden = false;
        m_list->add(*dm_row);
        parent_row->Children.insert(dm_row);
    }
}

void ChannelList::UpdateListingInternal() {
    std::unordered_set<Snowflake> guilds = Abaddon::Get().GetDiscordClient().GetGuildsID();

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

void ChannelList::on_guild_menu_move_up() {
    auto tmp = m_list->get_selected_row();
    auto row = dynamic_cast<ChannelListRow *>(tmp);
    if (row != nullptr)
        m_signal_action_guild_move_up.emit(row->ID);
}

void ChannelList::on_guild_menu_move_down() {
    auto tmp = m_list->get_selected_row();
    auto row = dynamic_cast<ChannelListRow *>(tmp);
    if (row != nullptr)
        m_signal_action_guild_move_down.emit(row->ID);
}

void ChannelList::on_guild_menu_copyid() {
    auto tmp = m_list->get_selected_row();
    auto row = dynamic_cast<ChannelListRow *>(tmp);
    if (row != nullptr)
        Gtk::Clipboard::get()->set_text(std::to_string(row->ID));
}

void ChannelList::on_guild_menu_leave() {
    auto row = dynamic_cast<ChannelListRow *>(m_list->get_selected_row());
    if (row != nullptr)
        m_signal_action_guild_leave.emit(row->ID);
}

void ChannelList::AttachGuildMenuHandler(Gtk::ListBoxRow *row) {
    row->signal_button_press_event().connect([&, row](GdkEventButton *e) -> bool {
        if (e->type == GDK_BUTTON_PRESS && e->button == GDK_BUTTON_SECONDARY) {
            auto grow = dynamic_cast<ChannelListRowGuild *>(row);
            if (grow != nullptr) {
                m_list->select_row(*row);
                m_guild_menu_up->set_sensitive(grow->GuildIndex != 0);
                m_guild_menu_down->set_sensitive(grow->GuildIndex != m_guild_count - 1);
                m_guild_menu.popup_at_pointer(reinterpret_cast<const GdkEvent *>(e));
            }
            return true;
        }

        return false;
    });
}

void ChannelList::on_channel_menu_copyid() {
    auto tmp = m_list->get_selected_row();
    auto row = dynamic_cast<ChannelListRow *>(tmp);
    if (row != nullptr)
        Gtk::Clipboard::get()->set_text(std::to_string(row->ID));
}

void ChannelList::AttachChannelMenuHandler(Gtk::ListBoxRow *row) {
    row->signal_button_press_event().connect([&, row](GdkEventButton *e) -> bool {
        if (e->type == GDK_BUTTON_PRESS && e->button == GDK_BUTTON_SECONDARY) {
            auto grow = dynamic_cast<ChannelListRow *>(row);
            if (grow != nullptr) {
                m_list->select_row(*row);
                m_channel_menu.popup_at_pointer(reinterpret_cast<const GdkEvent *>(e));
            }
            return true;
        }

        return false;
    });
}

ChannelList::type_signal_action_channel_item_select ChannelList::signal_action_channel_item_select() {
    return m_signal_action_channel_item_select;
}

ChannelList::type_signal_action_guild_move_up ChannelList::signal_action_guild_move_up() {
    return m_signal_action_guild_move_up;
}

ChannelList::type_signal_action_guild_move_down ChannelList::signal_action_guild_move_down() {
    return m_signal_action_guild_move_down;
}

ChannelList::type_signal_action_guild_leave ChannelList::signal_action_guild_leave() {
    return m_signal_action_guild_leave;
}
