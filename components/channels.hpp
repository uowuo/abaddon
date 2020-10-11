#pragma once
#include <gtkmm.h>
#include <string>
#include <queue>
#include <mutex>
#include <unordered_set>
#include <sigc++/sigc++.h>
#include "../discord/discord.hpp"

class ChannelListRow : public Gtk::ListBoxRow {
public:
    bool IsUserCollapsed;
    bool IsHidden;
    Snowflake ID;
    std::unordered_set<ChannelListRow *> Children;

    typedef sigc::signal<void> type_signal_list_collapse;
    typedef sigc::signal<void> type_signal_list_uncollapse;

    type_signal_list_collapse signal_list_collapse();
    type_signal_list_uncollapse signal_list_uncollapse();

protected:
    type_signal_list_collapse m_signal_list_collapse;
    type_signal_list_uncollapse m_signal_list_uncollapse;
};

class ChannelListRowDMHeader : public ChannelListRow {
public:
    ChannelListRowDMHeader();

protected:
    Gtk::EventBox *m_ev;
    Gtk::Box *m_box;
    Gtk::Label *m_lbl;
};

class ChannelListRowDMChannel : public ChannelListRow {
public:
    ChannelListRowDMChannel(const Channel *data);

protected:
    void OnImageLoad(Glib::RefPtr<Gdk::Pixbuf> buf);

    Gtk::EventBox *m_ev;
    Gtk::Box *m_box;
    Gtk::Label *m_lbl;
    Gtk::Image *m_icon = nullptr;
};

class ChannelListRowGuild : public ChannelListRow {
public:
    ChannelListRowGuild(const Guild *data);

    int GuildIndex;

protected:
    void OnImageLoad(Glib::RefPtr<Gdk::Pixbuf> buf);

    Gtk::EventBox *m_ev;
    Gtk::Box *m_box;
    Gtk::Label *m_lbl;
    Gtk::Image *m_icon;
};

class ChannelListRowCategory : public ChannelListRow {
public:
    ChannelListRowCategory(const Channel *data);

protected:
    Gtk::EventBox *m_ev;
    Gtk::Box *m_box;
    Gtk::Label *m_lbl;
    Gtk::Arrow *m_arrow;
};

class ChannelListRowChannel : public ChannelListRow {
public:
    ChannelListRowChannel(const Channel *data);

protected:
    Gtk::EventBox *m_ev;
    Gtk::Box *m_box;
    Gtk::Label *m_lbl;
};

class ChannelList {
public:
    ChannelList();
    Gtk::Widget *GetRoot() const;
    void UpdateListing();
    void Clear();

protected:
    Gtk::ListBox *m_list;
    Gtk::ScrolledWindow *m_main;

    void on_row_activated(Gtk::ListBoxRow *row);

    int m_guild_count;
    Gtk::Menu m_guild_menu;
    Gtk::MenuItem *m_guild_menu_up;
    Gtk::MenuItem *m_guild_menu_down;
    Gtk::MenuItem *m_guild_menu_copyid;
    Gtk::MenuItem *m_guild_menu_leave;
    void on_guild_menu_move_up();
    void on_guild_menu_move_down();
    void on_guild_menu_copyid();
    void on_guild_menu_leave();

    Gtk::Menu m_channel_menu;
    Gtk::MenuItem *m_channel_menu_copyid;
    void on_channel_menu_copyid();

    Glib::Dispatcher m_update_dispatcher;
    //mutable std::mutex m_update_mutex;
    //std::queue<std::unordered_set<Snowflake>> m_update_queue;
    void AddPrivateChannels(); // retard moment
    void UpdateListingInternal();
    void AttachGuildMenuHandler(Gtk::ListBoxRow *row);
    void AttachChannelMenuHandler(Gtk::ListBoxRow *row);

public:
    typedef sigc::signal<void, Snowflake> type_signal_action_channel_item_select;
    typedef sigc::signal<void, Snowflake> type_signal_action_guild_move_up;
    typedef sigc::signal<void, Snowflake> type_signal_action_guild_move_down;
    typedef sigc::signal<void, Snowflake> type_signal_action_guild_leave;

    type_signal_action_channel_item_select signal_action_channel_item_select();
    type_signal_action_guild_move_up signal_action_guild_move_up();
    type_signal_action_guild_move_down signal_action_guild_move_down();
    type_signal_action_guild_leave signal_action_guild_leave();

protected:
    type_signal_action_channel_item_select m_signal_action_channel_item_select;
    type_signal_action_guild_move_up m_signal_action_guild_move_up;
    type_signal_action_guild_move_down m_signal_action_guild_move_down;
    type_signal_action_guild_leave m_signal_action_guild_leave;
};
