#pragma once
#include <gtkmm.h>
#include "discord/objects.hpp"

class MutualFriendItem : public Gtk::Box {
public:
    MutualFriendItem(const UserData &user);

private:
    Gtk::Image m_avatar;
    Gtk::Label m_name;
};

class MutualFriendsPane : public Gtk::ScrolledWindow {
public:
    MutualFriendsPane(Snowflake id);

    Snowflake UserID;

private:
    void OnMap();

    bool m_requested = false;

    void OnFetchRelationships(const std::vector<UserData> &users);

    Gtk::ListBox m_list;
};
