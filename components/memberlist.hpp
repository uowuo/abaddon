#pragma once
#include <gtkmm.h>
#include <mutex>
#include <unordered_map>
#include "../discord/discord.hpp"

class MemberListUserRow : public Gtk::ListBoxRow {
public:
    MemberListUserRow(Snowflake guild_id, const User *data);
    void SetAvatarFromPixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf);

    Snowflake ID;

private:
    Gtk::EventBox *m_ev;
    Gtk::Box *m_box;
    Gtk::Image *m_avatar;
    Gtk::Label *m_label;
};

class MemberList {
public:
    MemberList();
    Gtk::Widget *GetRoot() const;

    void UpdateMemberList();
    void Clear();
    void SetActiveChannel(Snowflake id);

private:
    void on_copy_id_activate();
    void on_insert_mention_activate();
    void on_kick_activate();
    void on_ban_activate();

    void UpdateMemberListInternal();
    void AttachUserMenuHandler(Gtk::ListBoxRow *row, Snowflake id);

    Gtk::Menu m_menu;
    Gtk::MenuItem *m_menu_copy_id;
    Gtk::MenuItem *m_menu_insert_mention;
    Gtk::MenuItem *m_menu_kick;
    Gtk::MenuItem *m_menu_ban;
    Gtk::ListBoxRow *m_row_menu_target = nullptr; // maybe hacky

    std::mutex m_mutex;
    Glib::Dispatcher m_update_member_list_dispatcher;

    Gtk::ScrolledWindow *m_main;
    Gtk::ListBox *m_listbox;

    Snowflake m_guild_id;
    Snowflake m_chan_id;

    std::unordered_map<Snowflake, Gtk::ListBoxRow *> m_id_to_row;

public:
    typedef sigc::signal<void, Snowflake> type_signal_action_insert_mention;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_kick; // user_id, guild_id
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_ban;  // ^

    type_signal_action_insert_mention signal_action_insert_mention();
    type_signal_action_kick signal_action_kick();
    type_signal_action_ban signal_action_ban();

private:
    type_signal_action_insert_mention m_signal_action_insert_mention;
    type_signal_action_kick m_signal_action_kick;
    type_signal_action_ban m_signal_action_ban;
};
