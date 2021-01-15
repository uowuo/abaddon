#include "guildsettingswindow.hpp"
#include "../abaddon.hpp"

GuildSettingsWindow::GuildSettingsWindow(Snowflake id)
    : m_main(Gtk::ORIENTATION_VERTICAL)
    , GuildID(id)
    , m_pane_info(id) {
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

    if (guild.HasIcon()) {
        Abaddon::Get().GetImageManager().LoadFromURL(guild.GetIconURL(), sigc::mem_fun(*this, &GuildSettingsWindow::set_icon));
    }

    m_switcher.set_stack(m_stack);
    m_switcher.set_halign(Gtk::ALIGN_CENTER);
    m_switcher.set_hexpand(true);
    m_switcher.set_margin_top(10);
    m_switcher.show();

    m_pane_info.show();

    m_stack.add(m_pane_info, "info", "Info");
    m_stack.show();

    m_main.add(m_switcher);
    m_main.add(m_stack);
    m_main.show();
    add(m_main);
}
