#pragma once
#include <gtkmm.h>
#include <mutex>
#include <unordered_map>
#include "../discord/discord.hpp"
#include "lazyimage.hpp"

class MemberListUserRow : public Gtk::ListBoxRow {
public:
    MemberListUserRow(Snowflake guild_id, const UserData *data);

    Snowflake ID;

private:
    Gtk::EventBox *m_ev;
    Gtk::Box *m_box;
    LazyImage *m_avatar;
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
    void AttachUserMenuHandler(Gtk::ListBoxRow *row, Snowflake id);

    Gtk::ScrolledWindow *m_main;
    Gtk::ListBox *m_listbox;

    Snowflake m_guild_id;
    Snowflake m_chan_id;

    std::unordered_map<Snowflake, Gtk::ListBoxRow *> m_id_to_row;

public:
    typedef sigc::signal<void, const GdkEvent *, Snowflake, Snowflake> type_signal_action_show_user_menu;

    type_signal_action_show_user_menu signal_action_show_user_menu();

private:
    type_signal_action_show_user_menu m_signal_action_show_user_menu;
};
