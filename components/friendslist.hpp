#pragma once
#include <gtkmm.h>
#include "../discord/relationship.hpp"

class FriendsListAddComponent : public Gtk::Box {
public:
    FriendsListAddComponent();

private:
    Gtk::Label m_label;
    Gtk::Label m_status;
    Gtk::Entry m_entry;
    Gtk::Button m_add;
    Gtk::Box m_box;
};

class FriendsList : public Gtk::Box {
public:
    FriendsList();

private:
    enum FilterMode {
        FILTER_FRIENDS,
        FILTER_ONLINE,
        FILTER_PENDING,
        FILTER_BLOCKED,
    };

    FilterMode m_filter_mode;

    int ListSortFunc(Gtk::ListBoxRow *a, Gtk::ListBoxRow *b);
    bool ListFilterFunc(Gtk::ListBoxRow *row);

    FriendsListAddComponent m_add;
    Gtk::RadioButtonGroup m_group;
    Gtk::ButtonBox m_buttons;
    Gtk::ScrolledWindow m_scroll;
    Gtk::ListBox m_list;
};

class FriendsListFriendRow : public Gtk::ListBoxRow {
public:
    FriendsListFriendRow(RelationshipType type, const UserData &str);

    Snowflake ID;
    RelationshipType Type;
    Glib::ustring Name;

private:
    Gtk::Menu m_menu;
    Gtk::MenuItem m_remove; // or cancel or ignore

    using type_signal_remove = sigc::signal<void>;
    type_signal_remove m_signal_remove;

public:
    type_signal_remove signal_action_remove();
};

class FriendsListWindow : public Gtk::Window {
public:
    FriendsListWindow();

private:
    FriendsList m_friends;
};
