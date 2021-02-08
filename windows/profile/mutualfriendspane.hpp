#pragma once
#include <gtkmm.h>
#include "../../discord/objects.hpp"

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

    void SetMutualFriends(const std::vector<UserData> &users);

    Snowflake UserID;

private:
    Gtk::ListBox m_list;
};
