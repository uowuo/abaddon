#pragma once
#include <gtkmm.h>
#include "../../components/inotifyswitched.hpp"
#include "../../discord/objects.hpp"

class MutualFriendItem : public Gtk::Box {
public:
    MutualFriendItem(const UserData &user);

private:
    Gtk::Image m_avatar;
    Gtk::Label m_name;
};

class MutualFriendsPane : public Gtk::ScrolledWindow, public INotifySwitched {
public:
    MutualFriendsPane(Snowflake id);

    Snowflake UserID;

private:
    void on_switched_to() override;

    bool m_requested = false;

    void OnFetchRelationships(const std::vector<UserData> &users);

    Gtk::ListBox m_list;
};
