#pragma once
#include <gtkmm/listbox.h>
#include <gtkmm/menu.h>
#include "discord/snowflake.hpp"
#include "discord/usersettings.hpp"

class GuildListGuildItem;

class GuildList : public Gtk::ListBox {
public:
    GuildList();

    void UpdateListing();

private:
    void AddGuild(Snowflake id);
    void AddFolder(const UserSettingsGuildFoldersEntry &folder);
    void Clear();

    GuildListGuildItem *CreateGuildWidget(Snowflake id);

    // todo code duplication not good no sir
    Gtk::Menu m_menu_guild;
    Gtk::MenuItem m_menu_guild_copy_id;
    Gtk::MenuItem m_menu_guild_settings;
    Gtk::MenuItem m_menu_guild_leave;
    Gtk::MenuItem m_menu_guild_mark_as_read;
    Gtk::MenuItem m_menu_guild_toggle_mute;
    Snowflake m_menu_guild_target;

    void OnGuildSubmenuPopup();

public:
    using type_signal_guild_selected = sigc::signal<void, Snowflake>;
    using type_signal_dms_selected = sigc::signal<void>;
    using type_signal_action_guild_leave = sigc::signal<void, Snowflake>;
    using type_signal_action_guild_settings = sigc::signal<void, Snowflake>;

    type_signal_guild_selected signal_guild_selected();
    type_signal_dms_selected signal_dms_selected();
    type_signal_action_guild_leave signal_action_guild_leave();
    type_signal_action_guild_settings signal_action_guild_settings();

private:
    type_signal_guild_selected m_signal_guild_selected;
    type_signal_dms_selected m_signal_dms_selected;
    type_signal_action_guild_leave m_signal_action_guild_leave;
    type_signal_action_guild_settings m_signal_action_guild_settings;
};
