#pragma once
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/window.h>
#include "discord/objects.hpp"

class FriendsListAddComponent : public Gtk::Box {
public:
    FriendsListAddComponent();

private:
    void Submit();
    bool OnKeyPress(GdkEventKey *e);

    Gtk::Label m_label;
    Gtk::Label m_status;
    Gtk::Entry m_entry;
    Gtk::Button m_add;
    Gtk::Box m_box;

    bool m_requesting = false;
};

class FriendsListFriendRow;
class FriendsList : public Gtk::Box {
public:
    FriendsList();

private:
    FriendsListFriendRow *MakeRow(const UserData &user, RelationshipType type);

    void OnRelationshipAdd(const RelationshipAddData &data);
    void OnRelationshipRemove(Snowflake id, RelationshipType type);

    void OnActionAccept(Snowflake id);
    void OnActionRemove(Snowflake id);

    void PopulateRelationships();

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
    PresenceStatus Status;

private:
    void UpdatePresenceLabel();
    void OnPresenceUpdate(const UserData &user, PresenceStatus status);

    Gtk::Label *m_status_lbl;

    Gtk::Menu m_menu;
    Gtk::MenuItem m_remove; // or cancel or ignore
    Gtk::MenuItem m_accept; // incoming

    using type_signal_remove = sigc::signal<void>;
    using type_signal_accept = sigc::signal<void>;
    type_signal_remove m_signal_remove;
    type_signal_accept m_signal_accept;

public:
    type_signal_remove signal_action_remove();
    type_signal_accept signal_action_accept();
};

class FriendsListWindow : public Gtk::Window {
public:
    FriendsListWindow();

private:
    FriendsList m_friends;
};
