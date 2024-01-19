#pragma once

#include <gtkmm/stack.h>
#include <gtkmm/stackswitcher.h>
#include <gtkmm/window.h>

#include "discord/snowflake.hpp"
#include "guildsettings/infopane.hpp"
#include "guildsettings/banspane.hpp"
#include "guildsettings/invitespane.hpp"
#include "guildsettings/auditlogpane.hpp"
#include "guildsettings/memberspane.hpp"
#include "guildsettings/rolespane.hpp"
#include "guildsettings/emojispane.hpp"

class GuildSettingsWindow : public Gtk::Window {
public:
    GuildSettingsWindow(Snowflake id);

private:
    Gtk::Box m_main;
    Gtk::Stack m_stack;
    Gtk::StackSwitcher m_switcher;

    GuildSettingsInfoPane m_pane_info;
    GuildSettingsMembersPane m_pane_members;
    GuildSettingsRolesPane m_pane_roles;
    GuildSettingsBansPane m_pane_bans;
    GuildSettingsInvitesPane m_pane_invites;
    GuildSettingsEmojisPane m_pane_emojis;
    GuildSettingsAuditLogPane m_pane_audit_log;

    Snowflake GuildID;
};
