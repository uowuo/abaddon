#pragma once

#include <unordered_map>

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>

#include "discord/guild.hpp"
#include "components/draglistbox.hpp"

class GuildSettingsRolesPaneRolesListItem : public Gtk::ListBoxRow {
public:
    GuildSettingsRolesPaneRolesListItem(const GuildData &guild, const RoleData &role);

    Glib::ustring DisplayTerm;

    Snowflake GuildID;
    Snowflake RoleID;
    int Position;

private:
    void UpdateItem(const RoleData &role);
    void OnRoleUpdate(Snowflake guild_id, Snowflake role_id);

    Gtk::EventBox m_ev;
    Gtk::Label m_name;
};

class GuildSettingsRolesPaneRoles : public Gtk::Box {
public:
    GuildSettingsRolesPaneRoles(Snowflake guild_id);

private:
    void OnRoleCreate(Snowflake guild_id, Snowflake role_id);
    void OnRoleDelete(Snowflake guild_id, Snowflake role_id);

    Snowflake GuildID;

    Gtk::Entry m_search;
    Gtk::ScrolledWindow m_list_scroll;
    DragListBox m_list;

    typedef sigc::signal<void, Snowflake /* role_id */> type_signal_role_select;
    type_signal_role_select m_signal_role_select;

public:
    std::unordered_map<Snowflake, GuildSettingsRolesPaneRolesListItem *> m_rows;
    type_signal_role_select signal_role_select();
};

class GuildSettingsRolesPanePermItem : public Gtk::CheckButton {
public:
    GuildSettingsRolesPanePermItem(Permission perm);

private:
    Permission m_permission;

    typedef sigc::signal<void, Permission, bool> type_signal_permission_click;

    type_signal_permission_click m_signal_permission;

public:
    type_signal_permission_click signal_permission_click();
};

class GuildSettingsRolesPaneInfo : public Gtk::ScrolledWindow {
public:
    GuildSettingsRolesPaneInfo(Snowflake guild_id);

    void SetRole(const RoleData &role);

private:
    void OnRoleUpdate(Snowflake guild_id, Snowflake role_id);
    void OnPermissionToggle(Permission perm, bool new_set);

    void UpdateRoleName();

    Snowflake GuildID;
    Snowflake RoleID;

    Permission m_perms;

    std::vector<sigc::connection> m_update_connections;

    Gtk::Box m_layout;
    Gtk::Box m_meta;
    Gtk::Entry m_role_name;
    Gtk::ColorButton m_color_button;
    Gtk::Grid m_grid;

    std::unordered_map<Permission, GuildSettingsRolesPanePermItem *> m_perm_items;
};

class GuildSettingsRolesPane : public Gtk::Box {
public:
    GuildSettingsRolesPane(Snowflake id);

private:
    void OnRoleSelect(Snowflake role_id);

    Snowflake GuildID;

    Gtk::Box m_layout;
    GuildSettingsRolesPaneRoles m_roles_list;
    GuildSettingsRolesPaneInfo m_roles_perms;
};
