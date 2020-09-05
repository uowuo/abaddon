#pragma once
#include <gtkmm.h>
#include <mutex>
#include "../discord/discord.hpp"

class Abaddon;
class MemberList {
public:
    MemberList();
    Gtk::Widget *GetRoot() const;

    void UpdateMemberList();
    void SetActiveChannel(Snowflake id);

    void SetAbaddon(Abaddon *ptr);

private:
    void UpdateMemberListInternal();

    std::mutex m_mutex;
    Glib::Dispatcher m_update_member_list_dispatcher;

    Gtk::ScrolledWindow *m_main;
    Gtk::ListBox *m_listbox;

    Snowflake m_guild_id;
    Snowflake m_chan_id;
    Abaddon *m_abaddon = nullptr;
};
