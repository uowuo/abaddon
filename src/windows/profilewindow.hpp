#pragma once

#include <gtkmm/stack.h>
#include <gtkmm/stackswitcher.h>
#include <gtkmm/window.h>

#include "discord/snowflake.hpp"
#include "profile/userinfopane.hpp"
#include "profile/mutualguildspane.hpp"
#include "profile/mutualfriendspane.hpp"

class ProfileWindow : public Gtk::Window {
public:
    ProfileWindow(Snowflake user_id);

    Snowflake ID;

private:
    void OnFetchProfile(const UserProfileData &data);

    Gtk::Box m_main;
    Gtk::Box m_upper;
    Gtk::Box m_badges;
    Gtk::Box m_name_box;
    Gtk::ScrolledWindow m_badges_scroll;
    Gtk::EventBox m_avatar_ev;
    Gtk::Image m_avatar;
    Gtk::Label m_displayname;
    Gtk::Label m_username;
    Gtk::ScrolledWindow m_scroll;
    Gtk::Stack m_stack;
    Gtk::StackSwitcher m_switcher;

    ProfileUserInfoPane m_pane_info;
    UserMutualGuildsPane m_pane_guilds;
    MutualFriendsPane m_pane_friends;
};
