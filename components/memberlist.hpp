#pragma once
#include <gtkmm.h>
#include <mutex>
#include "../discord/discord.hpp"

class MemberList {
public:
    class MemberListUserRow : public Gtk::ListBoxRow {
    public:
        Snowflake ID;
    };

    MemberList();
    Gtk::Widget *GetRoot() const;

    void UpdateMemberList();
    void SetActiveChannel(Snowflake id);

private:
    void on_copy_id_activate();
    void on_insert_mention_activate();

    void UpdateMemberListInternal();
    void AttachUserMenuHandler(Gtk::ListBoxRow *row, Snowflake id);

    Gtk::Menu m_menu;
    Gtk::MenuItem *m_menu_copy_id;
    Gtk::MenuItem *m_menu_insert_mention;
    Gtk::ListBoxRow *m_row_menu_target = nullptr; // maybe hacky

    std::mutex m_mutex;
    Glib::Dispatcher m_update_member_list_dispatcher;

    Gtk::ScrolledWindow *m_main;
    Gtk::ListBox *m_listbox;

    Snowflake m_guild_id;
    Snowflake m_chan_id;
};
