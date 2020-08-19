#include "channels.hpp"
#include <algorithm>
#include <map>
#include <unordered_map>
#include "../abaddon.hpp"

ChannelList::ChannelList() {
    m_main = Gtk::manage(new Gtk::ScrolledWindow);
    m_list = Gtk::manage(new Gtk::ListBox);
    m_list->set_activate_on_single_click(true);

    m_list->signal_row_activated().connect(sigc::mem_fun(*this, &ChannelList::on_row_activated));

    m_main->add(*m_list);
    m_main->show_all();

    m_update_dispatcher.connect(sigc::mem_fun(*this, &ChannelList::SetListingFromGuildsInternal));
}

void ChannelList::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
}

Gtk::Widget *ChannelList::GetRoot() const {
    return m_main;
}

void ChannelList::SetListingFromGuilds(const DiscordClient::Guilds_t &guilds) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_update_queue.push(guilds);
    m_update_dispatcher.emit();
}

void ChannelList::on_row_activated(Gtk::ListBoxRow *row) {
    auto &info = m_infos[row];
    bool new_collapsed = !info.IsUserCollapsed;
    info.IsUserCollapsed = new_collapsed;

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

void ChannelList::SetListingFromGuildsInternal() {
    DiscordClient::Guilds_t *guilds;
    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        guilds = &m_update_queue.front();
    }

    auto children = m_list->get_children();
    auto it = children.begin();

    while (it != children.end()) {
        delete *it;
        it++;
    }

    auto &settings = m_abaddon->GetDiscordClient().GetUserSettings();

    std::vector<std::pair<Snowflake, GuildData>> sorted_guilds;

    if (settings.GuildPositions.size()) {
        for (const auto &id : settings.GuildPositions) {
            auto &guild = (*guilds)[id];
            sorted_guilds.push_back(std::make_pair(id, guild));
        }
    } else { // default sort is alphabetic
        for (auto &it : *guilds)
            sorted_guilds.push_back(it);
        std::sort(sorted_guilds.begin(), sorted_guilds.end(), [&](auto &a, auto &b) -> bool {
            std::string &s1 = a.second.Name;
            std::string &s2 = b.second.Name;

            if (s1.empty() || s2.empty())
                return s1 < s2;

            bool ac[] = {
                !isalnum(s1[0]),
                !isalnum(s2[0]),
                isdigit(s1[0]),
                isdigit(s2[0]),
                isalpha(s1[0]),
                isalpha(s2[0]),
            };

            if ((ac[0] && ac[1]) || (ac[2] && ac[3]) || (ac[4] && ac[5]))
                return s1 < s2;

            return ac[0] || ac[5];
        });
    }

    // map each category to its channels
    std::unordered_map<Snowflake, std::vector<const ChannelData *>> cat_to_channels;
    std::unordered_map<Snowflake, std::vector<const ChannelData *>> orphan_channels;
    for (const auto &[gid, guild] : *guilds) {
        for (const auto &chan : guild.Channels) {
            switch (chan.Type) {
                case ChannelType::GUILD_TEXT: {
                    if (chan.ParentID.IsValid()) {
                        auto it = cat_to_channels.find(chan.ParentID);
                        if (it == cat_to_channels.end())
                            cat_to_channels[chan.ParentID] = std::vector<const ChannelData *>();
                        cat_to_channels[chan.ParentID].push_back(&chan);
                    } else {
                        auto it = orphan_channels.find(gid);
                        if (it == orphan_channels.end())
                            orphan_channels[gid] = std::vector<const ChannelData *>();
                        orphan_channels[gid].push_back(&chan);
                    }
                } break;
                default:
                    break;
            }
        }
    }

    auto add_channel = [&](const Snowflake &id, const ChannelData &channel) -> Gtk::ListBoxRow * {
        auto *channel_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *channel_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        auto *channel_label = Gtk::manage(new Gtk::Label);
        channel_label->set_text("#" + channel.Name);
        channel_box->set_halign(Gtk::ALIGN_START);
        channel_box->pack_start(*channel_label);
        channel_row->add(*channel_box);
        channel_row->show_all_children();
        m_list->add(*channel_row);

        ListItemInfo info;
        info.ID = id;
        info.IsUserCollapsed = false;
        info.IsHidden = false;

        m_infos[channel_row] = std::move(info);
        return channel_row;
    };

    auto add_category = [&](const Snowflake &id, const ChannelData &channel) -> Gtk::ListBoxRow * {
        auto *category_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *category_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        auto *category_label = Gtk::manage(new Gtk::Label);
        auto *category_arrow = Gtk::manage(new Gtk::Arrow(Gtk::ARROW_DOWN, Gtk::SHADOW_NONE));
        category_label->set_text(channel.Name);
        category_box->set_halign(Gtk::ALIGN_START);
        category_box->pack_start(*category_arrow);
        category_box->pack_start(*category_label);
        category_row->add(*category_box);
        category_row->show_all_children();
        m_list->add(*category_row);

        ListItemInfo info;
        info.ID = id;
        info.IsUserCollapsed = false;
        info.IsHidden = true;
        info.CatArrow = category_arrow;

        if (cat_to_channels.find(id) != cat_to_channels.end()) {
            std::map<int, const ChannelData *> sorted_channels;

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

    auto add_guild = [&](const Snowflake &id, const GuildData &guild) {
        auto *guild_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *guild_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        auto *guild_label = Gtk::manage(new Gtk::Label);
        guild_label->set_markup("<b>" + Glib::Markup::escape_text(guild.Name) + "</b>");
        guild_box->set_halign(Gtk::ALIGN_START);
        guild_box->pack_start(*guild_label);
        guild_row->add(*guild_box);
        guild_row->show_all();
        m_list->add(*guild_row);

        ListItemInfo info;
        info.ID = id;
        info.IsUserCollapsed = true;
        info.IsHidden = false;

        if (orphan_channels.find(id) != orphan_channels.end()) {
            std::map<int, const ChannelData *> sorted_orphans;

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

        std::map<int, const ChannelData *> sorted_categories;

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

    for (const auto &[id, guild] : sorted_guilds) {
        add_guild(id, guild);
    }

    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        m_update_queue.pop();
    }
}
