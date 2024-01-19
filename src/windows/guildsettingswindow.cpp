#include "guildsettingswindow.hpp"

#include "abaddon.hpp"

GuildSettingsWindow::GuildSettingsWindow(Snowflake id)
    : m_main(Gtk::ORIENTATION_VERTICAL)
    , m_pane_info(id)
    , m_pane_members(id)
    , m_pane_roles(id)
    , m_pane_bans(id)
    , m_pane_invites(id)
    , m_pane_emojis(id)
    , m_pane_audit_log(id)
    , GuildID(id) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto guild = *discord.GetGuild(id);

    auto guild_update_cb = [this](Snowflake id) {
        if (id != GuildID) return;
        const auto guild = *Abaddon::Get().GetDiscordClient().GetGuild(id);
        set_title(guild.Name);
        if (guild.HasIcon())
            Abaddon::Get().GetImageManager().LoadFromURL(guild.GetIconURL(), sigc::mem_fun(*this, &GuildSettingsWindow::set_icon));
    };
    discord.signal_guild_update().connect(sigc::track_obj(guild_update_cb, *this));

    set_name("guild-settings");
    set_default_size(800, 600);
    set_title(guild.Name);
    set_position(Gtk::WIN_POS_CENTER);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");
    get_style_context()->add_class("guild-settings-window");

    if (guild.HasIcon()) {
        Abaddon::Get().GetImageManager().LoadFromURL(guild.GetIconURL(), sigc::mem_fun(*this, &GuildSettingsWindow::set_icon));
    }

    m_switcher.set_stack(m_stack);
    m_switcher.set_halign(Gtk::ALIGN_CENTER);
    m_switcher.set_hexpand(true);
    m_switcher.set_margin_top(10);
    m_switcher.show();

    m_pane_info.show();
    m_pane_members.show();
    m_pane_roles.show();
    m_pane_bans.show();
    m_pane_invites.show();
    m_pane_emojis.show();
    m_pane_audit_log.show();

    m_stack.set_transition_duration(100);
    m_stack.set_transition_type(Gtk::STACK_TRANSITION_TYPE_CROSSFADE);
    m_stack.set_margin_top(10);
    m_stack.set_margin_bottom(10);
    m_stack.set_margin_left(10);
    m_stack.set_margin_right(10);

    const auto self_id = discord.GetUserData().ID;

    m_stack.add(m_pane_info, "info", "Info");
    m_stack.add(m_pane_members, "members", "Members");
    m_stack.add(m_pane_roles, "roles", "Roles");
    m_stack.add(m_pane_bans, "bans", "Bans");
    if (discord.HasGuildPermission(self_id, GuildID, Permission::MANAGE_GUILD))
        m_stack.add(m_pane_invites, "invites", "Invites");
    m_stack.add(m_pane_emojis, "emojis", "Emojis");
    if (discord.HasGuildPermission(self_id, GuildID, Permission::VIEW_AUDIT_LOG))
        m_stack.add(m_pane_audit_log, "audit-log", "Audit Log");
    m_stack.show();

    m_main.add(m_switcher);
    m_main.add(m_stack);
    m_main.show();
    add(m_main);
}
