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
        enum ListItemType {
            Guild,
            Category,
            Channel,
        };
        int GuildIndex;
        Snowflake ID;
        std::unordered_set<Gtk::ListBoxRow *> Children;
        bool IsUserCollapsed;
        bool IsHidden;
        ListItemType Type;
        // for categories
        Gtk::Arrow *CatArrow = nullptr;
    };
    std::unordered_map<Gtk::ListBoxRow *, ListItemInfo> m_infos;

    void on_row_activated(Gtk::ListBoxRow *row);

    int m_guild_count;
    Gtk::Menu m_guild_menu;
    Gtk::MenuItem *m_guild_menu_up;
    Gtk::MenuItem *m_guild_menu_down;
    Gtk::MenuItem *m_guild_menu_copyid;
    void on_menu_move_up();
    void on_menu_move_down();
    void on_menu_copyid();

    Glib::Dispatcher m_update_dispatcher;
    mutable std::mutex m_update_mutex;
    std::queue<DiscordClient::Guilds_t> m_update_queue;
    void AddPrivateChannels(); // retard moment
    void SetListingFromGuildsInternal();
    void AttachMenuHandler(Gtk::ListBoxRow *row);

    Abaddon *m_abaddon = nullptr;
};
