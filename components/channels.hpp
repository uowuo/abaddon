#pragma once
#include <gtkmm.h>
#include <string>
#include <queue>
#include <mutex>
#include <unordered_set>
#include "../discord/discord.hpp"

class Abaddon;
class ChannelList {
public:
    ChannelList();
    Gtk::Widget *GetRoot() const;
    void SetListingFromGuilds(const DiscordClient::Guilds_t &guilds);
    void ClearListing();

    void SetAbaddon(Abaddon *ptr);

protected:
    Gtk::ListBox *m_list;
    Gtk::ScrolledWindow *m_main;

    struct ListItemInfo {
        Snowflake ID;
        std::unordered_set<Gtk::ListBoxRow *> Children;
        bool IsUserCollapsed;
        bool IsHidden;
        // for categories
        Gtk::Arrow *CatArrow = nullptr;
    };
    std::unordered_map<Gtk::ListBoxRow *, ListItemInfo> m_infos;

    void on_row_activated(Gtk::ListBoxRow *row);

    Glib::Dispatcher m_update_dispatcher;
    mutable std::mutex m_update_mutex;
    std::queue<DiscordClient::Guilds_t> m_update_queue;
    void SetListingFromGuildsInternal();

    Abaddon *m_abaddon = nullptr;
};
