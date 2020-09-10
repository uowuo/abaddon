#include "channels.hpp"
#include <algorithm>
#include <map>
#include <unordered_map>
#include "../abaddon.hpp"

ChannelList::ChannelList() {
    m_main = Gtk::manage(new Gtk::ScrolledWindow);
    m_list = Gtk::manage(new Gtk::ListBox);

    m_list->get_style_context()->add_class("channel-list");

    m_guild_menu_up = Gtk::manage(new Gtk::MenuItem("Move _Up", true));
    m_guild_menu_up->signal_activate().connect(sigc::mem_fun(*this, &ChannelList::on_menu_move_up));
    m_guild_menu.append(*m_guild_menu_up);

    m_guild_menu_down = Gtk::manage(new Gtk::MenuItem("Move _Down", true));
    m_guild_menu_down->signal_activate().connect(sigc::mem_fun(*this, &ChannelList::on_menu_move_down));
    m_guild_menu.append(*m_guild_menu_down);

    m_guild_menu_copyid = Gtk::manage(new Gtk::MenuItem("_Copy ID", true));
    m_guild_menu_copyid->signal_activate().connect(sigc::mem_fun(*this, &ChannelList::on_menu_copyid));
    m_guild_menu.append(*m_guild_menu_copyid);

    m_guild_menu.show_all();

    m_list->set_activate_on_single_click(true);
    m_list->signal_row_activated().connect(sigc::mem_fun(*this, &ChannelList::on_row_activated));

    m_main->add(*m_list);
    m_main->show_all();

    m_update_dispatcher.connect(sigc::mem_fun(*this, &ChannelList::SetListingFromGuildsInternal));
}

Gtk::Widget *ChannelList::GetRoot() const {
    return m_main;
}

void ChannelList::SetListingFromGuilds(const DiscordClient::guilds_type &guilds) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_update_queue.push(guilds);
    m_update_dispatcher.emit();
}

void ChannelList::Clear() {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_update_queue.push(DiscordClient::guilds_type());
    m_update_dispatcher.emit();
}

void ChannelList::on_row_activated(Gtk::ListBoxRow *row) {
    auto &info = m_infos[row];
    bool new_collapsed = !info.IsUserCollapsed;
    info.IsUserCollapsed = new_collapsed;

    if (info.Type == ListItemInfo::ListItemType::Channel) {
        m_signal_action_channel_item_select.emit(info.ID);
    }

    if (info.CatArrow != nullptr)
        info.CatArrow->set(new_collapsed ? Gtk::ARROW_RIGHT : Gtk::ARROW_DOWN, Gtk::SHADOW_NONE);

    if (new_collapsed) {
        std::function<void(Gtk::ListBoxRow *)> collapse_children = [&](Gtk::ListBoxRow *row) {
            for (auto child : m_infos[row].Children) {
                auto &row_info = m_infos[child];
                row_info.IsHidden = true;
                child->hide();
                collapse_children(child);
            }
        };
        collapse_children(row);
    } else {
        std::function<void(Gtk::ListBoxRow *)> restore_children = [&](Gtk::ListBoxRow *row) {
            auto &row_info = m_infos[row];
            row->show();
            row_info.IsHidden = false;
            if (!row_info.IsUserCollapsed)
                for (auto row : row_info.Children)
                    restore_children(row);
        };
        restore_children(row);
    }
}

void ChannelList::AddPrivateChannels() {
    auto dms = Abaddon::Get().GetDiscordClient().GetPrivateChannels();

    auto *parent_row = Gtk::manage(new Gtk::ListBoxRow);
    auto *parent_ev = Gtk::manage(new Gtk::EventBox);
    auto *parent_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    auto *parent_label = Gtk::manage(new Gtk::Label);
    parent_label->set_use_markup(true);
    parent_label->set_markup("<b>Direct Messages</b>");
    parent_box->set_halign(Gtk::ALIGN_START);
    parent_box->pack_start(*parent_label);
    parent_ev->add(*parent_box);
    parent_row->add(*parent_ev);
    parent_row->show_all();
    parent_row->get_style_context()->add_class("channel-row");
    parent_label->get_style_context()->add_class("channel-row-label");
    m_list->add(*parent_row);

    ListItemInfo parent_info;
    parent_info.IsUserCollapsed = true;
    parent_info.IsHidden = false;
    parent_info.Type = ListItemInfo::ListItemType::Guild; // good nuf

    for (const auto &dmid : dms) {
        auto *data = Abaddon::Get().GetDiscordClient().GetChannel(dmid);

        auto *dm_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *dm_ev = Gtk::manage(new Gtk::EventBox);
        auto *dm_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        auto *dm_label = Gtk::manage(new Gtk::Label);

        dm_row->get_style_context()->add_class("channel-row");
        dm_label->get_style_context()->add_class("channel-row-label");

        if (data->Type == ChannelType::DM)
            dm_label->set_text(data->Recipients[0].Username);
        else
            dm_label->set_text(std::to_string(data->Recipients.size()) + " users");
        dm_box->set_halign(Gtk::ALIGN_START);
        dm_box->pack_start(*dm_label);
        dm_ev->add(*dm_box);
        dm_row->add(*dm_ev);
        dm_row->show_all_children();
        m_list->add(*dm_row);

        ListItemInfo info;
        info.ID = dmid;
        info.IsUserCollapsed = false;
        info.IsHidden = false;
        info.Type = ListItemInfo::ListItemType::Channel;
        m_infos[dm_row] = std::move(info);
        parent_info.Children.insert(dm_row);
    }

    m_infos[parent_row] = std::move(parent_info);
}

void ChannelList::SetListingFromGuildsInternal() {
    DiscordClient::guilds_type guilds;
    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        guilds = m_update_queue.front();
        m_update_queue.pop();
    }

    auto children = m_list->get_children();
    auto it = children.begin();

    while (it != children.end()) {
        delete *it;
        it++;
    }

    m_guild_count = 0;

    AddPrivateChannels();

    // map each category to its channels

    std::unordered_map<Snowflake, std::vector<const Channel *>> cat_to_channels;
    std::unordered_map<Snowflake, std::vector<const Channel *>> orphan_channels;
    for (const auto &[gid, guild] : guilds) {
        for (const auto &chan : guild.Channels) {
            switch (chan.Type) {
                case ChannelType::GUILD_TEXT: {
                    if (chan.ParentID.IsValid()) {
                        auto it = cat_to_channels.find(chan.ParentID);
                        if (it == cat_to_channels.end())
                            cat_to_channels[chan.ParentID] = std::vector<const Channel *>();
                        cat_to_channels[chan.ParentID].push_back(&chan);
                    } else {
                        auto it = orphan_channels.find(gid);
                        if (it == orphan_channels.end())
                            orphan_channels[gid] = std::vector<const Channel *>();
                        orphan_channels[gid].push_back(&chan);
                    }
                } break;
                default:
                    break;
            }
        }
    }

    auto add_channel = [&](const Snowflake &id, const Channel &channel) -> Gtk::ListBoxRow * {
        auto *channel_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *channel_ev = Gtk::manage(new Gtk::EventBox);
        auto *channel_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        auto *channel_label = Gtk::manage(new Gtk::Label);

        channel_row->get_style_context()->add_class("channel-row");
        channel_row->get_style_context()->add_class("channel-row-channel");
        channel_label->get_style_context()->add_class("channel-row-label");

        channel_label->set_text("#" + channel.Name);
        channel_box->set_halign(Gtk::ALIGN_START);
        channel_box->pack_start(*channel_label);
        channel_ev->add(*channel_box);
        channel_row->add(*channel_ev);
        channel_row->show_all_children();
        m_list->add(*channel_row);

        ListItemInfo info;
        info.ID = id;
        info.IsUserCollapsed = false;
        info.IsHidden = false;
        info.Type = ListItemInfo::ListItemType::Channel;

        m_infos[channel_row] = std::move(info);
        return channel_row;
    };

    auto add_category = [&](const Snowflake &id, const Channel &channel) -> Gtk::ListBoxRow * {
        auto *category_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *category_ev = Gtk::manage(new Gtk::EventBox);
        auto *category_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        auto *category_label = Gtk::manage(new Gtk::Label);
        auto *category_arrow = Gtk::manage(new Gtk::Arrow(Gtk::ARROW_DOWN, Gtk::SHADOW_NONE));

        category_row->get_style_context()->add_class("channel-row");
        category_row->get_style_context()->add_class("channel-row-category");
        category_label->get_style_context()->add_class("channel-row-label");

        category_label->set_text(channel.Name);
        category_box->set_halign(Gtk::ALIGN_START);
        category_box->pack_start(*category_arrow);
        category_box->pack_start(*category_label);
        category_ev->add(*category_box);
        category_row->add(*category_ev);
        category_row->show_all_children();
        m_list->add(*category_row);

        ListItemInfo info;
        info.ID = id;
        info.IsUserCollapsed = false;
        info.IsHidden = true;
        info.CatArrow = category_arrow;
        info.Type = ListItemInfo::ListItemType::Category;

        if (cat_to_channels.find(id) != cat_to_channels.end()) {
            std::map<int, const Channel *> sorted_channels;

            for (const auto channel : cat_to_channels[id]) {
                assert(channel->Position != -1);
                sorted_channels[channel->Position] = channel;
            }

            for (const auto &[pos, channel] : sorted_channels) {
                auto channel_row = add_channel(channel->ID, *channel);
                info.Children.insert(channel_row);
            }
        }

        m_infos[category_row] = std::move(info);
        return category_row;
    };

    auto add_guild = [&](const Snowflake &id, const Guild &guild) {
        auto *guild_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *guild_ev = Gtk::manage(new Gtk::EventBox);
        auto *guild_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        auto *guild_label = Gtk::manage(new Gtk::Label);

        guild_row->get_style_context()->add_class("channel-row");
        guild_row->get_style_context()->add_class("channel-row-guild");
        guild_label->get_style_context()->add_class("channel-row-label");

        guild_label->set_markup("<b>" + Glib::Markup::escape_text(guild.Name) + "</b>");
        guild_box->set_halign(Gtk::ALIGN_START);
        guild_box->pack_start(*guild_label);
        guild_ev->add(*guild_box);
        guild_row->add(*guild_ev);
        guild_row->show_all();
        m_list->add(*guild_row);
        AttachMenuHandler(guild_row);

        ListItemInfo info;
        info.ID = id;
        info.IsUserCollapsed = true;
        info.IsHidden = false;
        info.GuildIndex = m_guild_count++;
        info.Type = ListItemInfo::ListItemType::Guild;

        if (orphan_channels.find(id) != orphan_channels.end()) {
            std::map<int, const Channel *> sorted_orphans;

            for (const auto channel : orphan_channels[id]) {
                assert(channel->Position != -1); // can this trigger?
                sorted_orphans[channel->Position] = channel;
            }

            for (const auto &[pos, channel] : sorted_orphans) {
                auto channel_row = add_channel(channel->ID, *channel);
                m_infos[channel_row].IsHidden = true;
                info.Children.insert(channel_row);
            }
        }

        std::map<int, const Channel *> sorted_categories;

        for (const auto &channel : guild.Channels) {
            if (channel.Type == ChannelType::GUILD_CATEGORY) {
                assert(channel.Position != -1);
                sorted_categories[channel.Position] = &channel;
            }
        }

        for (const auto &[pos, channel] : sorted_categories) {
            if (channel->Type == ChannelType::GUILD_CATEGORY) {
                auto category_row = add_category(channel->ID, *channel);
                info.Children.insert(category_row);
            }
        }

        m_infos[guild_row] = std::move(info);
    };

    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto &sorted_guilds = discord.GetUserSortedGuilds();
    for (const auto &id : sorted_guilds) {
        add_guild(id, *discord.GetGuild(id));
    }
}

void ChannelList::on_menu_move_up() {
    auto row = m_list->get_selected_row();
    m_signal_action_guild_move_up.emit(m_infos[row].ID);
}

void ChannelList::on_menu_move_down() {
    auto row = m_list->get_selected_row();
    m_signal_action_guild_move_down.emit(m_infos[row].ID);
}

void ChannelList::on_menu_copyid() {
    auto row = m_list->get_selected_row();
    m_signal_action_guild_copy_id.emit(m_infos[row].ID);
}

void ChannelList::AttachMenuHandler(Gtk::ListBoxRow *row) {
    row->signal_button_press_event().connect([&, row](GdkEventButton *e) -> bool {
        if (e->type == GDK_BUTTON_PRESS && e->button == GDK_BUTTON_SECONDARY) {
            m_list->select_row(*row);
            m_guild_menu_up->set_sensitive(m_infos[row].GuildIndex != 0);
            m_guild_menu_down->set_sensitive(m_infos[row].GuildIndex != m_guild_count - 1);
            m_guild_menu.popup_at_pointer(reinterpret_cast<const GdkEvent *>(e));
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

ChannelList::type_signal_action_guild_copy_id ChannelList::signal_action_guild_copy_id() {
    return m_signal_action_guild_copy_id;
}
