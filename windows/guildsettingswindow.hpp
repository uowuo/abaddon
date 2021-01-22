#pragma once
#include <gtkmm.h>
#include "../discord/snowflake.hpp"
#include "guildsettings/infopane.hpp"
#include "guildsettings/banspane.hpp"
#include "guildsettings/invitespane.hpp"

class GuildSettingsWindow : public Gtk::Window {
public:
    GuildSettingsWindow(Snowflake id);

private:
    Gtk::Box m_main;
    Gtk::Stack m_stack;
    Gtk::StackSwitcher m_switcher;

    GuildSettingsInfoPane m_pane_info;
    GuildSettingsBansPane m_pane_bans;
    GuildSettingsInvitesPane m_pane_invites;

    Snowflake GuildID;
};
