#pragma once

#include <unordered_set>

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>

#include "discord/member.hpp"
#include "discord/guild.hpp"
#include "components/lazyimage.hpp"

class GuildSettingsMembersPaneRolesItem : public Gtk::ListBoxRow {
public:
    GuildSettingsMembersPaneRolesItem(bool sensitive, const RoleData &role);
    void SetChecked(bool checked);
    void SetToggleable(bool toggleable);
    void UpdateRoleData(const RoleData &role);

    Snowflake RoleID;
    int Position;

private:
    void UpdateLabel();
    void ComputeSensitivity();
    bool m_desired_sensitivity = true;

    RoleData m_role;

    Gtk::Box m_main;
    Gtk::CheckButton m_check;
    Gtk::Label m_label;

    // own thing so we can stop it from actually changing
    typedef sigc::signal<void, Snowflake, bool> type_signal_role_click;

    type_signal_role_click m_signal_role_click;

public:
    type_signal_role_click signal_role_click();
};

class GuildSettingsMembersPaneRoles : public Gtk::ScrolledWindow {
public:
    GuildSettingsMembersPaneRoles(Snowflake guild_id);

    void SetRoles(Snowflake user_id, const std::vector<Snowflake> &roles, bool is_owner);

private:
    void CreateRow(bool has_manage_roles, const RoleData &role, bool is_owner);

    void OnRoleToggle(Snowflake role_id, bool new_set);

    void OnRoleCreate(Snowflake guild_id, Snowflake role_id);
    void OnRoleUpdate(Snowflake guild_id, Snowflake role_id);
    void OnRoleDelete(Snowflake guild_id, Snowflake role_id);

    int m_hoisted_position = 0;

    std::vector<sigc::connection> m_update_connection;

    std::unordered_set<Snowflake> m_set_role_ids;

    Snowflake GuildID;
    Snowflake UserID;

    Gtk::ListBox m_list;

    std::unordered_map<Snowflake, GuildSettingsMembersPaneRolesItem *> m_rows;
};

class GuildSettingsMembersPaneInfo : public Gtk::ScrolledWindow {
public:
    GuildSettingsMembersPaneInfo(Snowflake guild_id);

    void SetUser(Snowflake user_id);

private:
    Snowflake GuildID;
    Snowflake UserID;

    Gtk::Label m_bot;
    Gtk::Label m_id;
    Gtk::Label m_created;
    Gtk::Label m_joined;
    Gtk::Label m_nickname;
    Gtk::Label m_boosting;
    GuildSettingsMembersPaneRoles m_roles;
    Gtk::Box m_box;
};

class GuildSettingsMembersPaneMembers : public Gtk::Box {
public:
    GuildSettingsMembersPaneMembers(Snowflake id);

private:
    Snowflake GuildID;

    Gtk::Entry m_search;
    Gtk::ScrolledWindow m_list_scroll;
    Gtk::ListBox m_list;

    typedef sigc::signal<void, Snowflake> type_signal_member_select;
    type_signal_member_select m_signal_member_select;

public:
    type_signal_member_select signal_member_select();
};

class GuildSettingsMembersListItem : public Gtk::ListBoxRow {
public:
    GuildSettingsMembersListItem(const GuildData &guild, const GuildMember &member);

    Glib::ustring DisplayTerm;

    Snowflake UserID;
    Snowflake GuildID;

private:
    void UpdateColor();

    Gtk::EventBox m_ev;
    LazyImage m_avatar;
    Gtk::Label m_name;
    Gtk::Box m_main;
    Gtk::Image *m_crown = nullptr;
};

class GuildSettingsMembersPane : public Gtk::Box {
public:
    GuildSettingsMembersPane(Snowflake id);

private:
    Snowflake GuildID;

    Gtk::Box m_layout;
    Gtk::Label m_note;
    GuildSettingsMembersPaneMembers m_member_list;
    GuildSettingsMembersPaneInfo m_member_info;
};
