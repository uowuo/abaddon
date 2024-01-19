#pragma once

#include <gtkmm/liststore.h>
#include <gtkmm/menu.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>

#include "discord/objects.hpp"

class GuildSettingsInvitesPane : public Gtk::ScrolledWindow {
public:
    GuildSettingsInvitesPane(Snowflake id);

private:
    void OnMap();

    bool m_requested = false;

    void AppendInvite(const InviteData &invite);
    void OnInviteFetch(const std::optional<InviteData> &invite);
    void OnInvitesFetch(const std::vector<InviteData> &invites);
    void OnInviteCreate(const InviteData &invite);
    void OnInviteDelete(const InviteDeleteObject &data);
    void OnMenuDelete();
    bool OnTreeButtonPress(GdkEventButton *event);

    Gtk::TreeView m_view;

    Snowflake GuildID;

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns();

        Gtk::TreeModelColumn<Glib::ustring> m_col_code;
        Gtk::TreeModelColumn<Glib::ustring> m_col_expires;
        Gtk::TreeModelColumn<Glib::ustring> m_col_inviter;
        Gtk::TreeModelColumn<Glib::ustring> m_col_temporary;
        Gtk::TreeModelColumn<int> m_col_uses;
        Gtk::TreeModelColumn<Glib::ustring> m_col_max_uses;
    };

    ModelColumns m_columns;
    Glib::RefPtr<Gtk::ListStore> m_model;

    Gtk::Menu m_menu;
    Gtk::MenuItem m_menu_delete;
};
