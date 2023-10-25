#include "guildlist.hpp"
#include "guildlistfolderitem.hpp"

GuildList::GuildList() {
    set_selection_mode(Gtk::SELECTION_NONE);
    show_all_children();
}

void GuildList::UpdateListing() {
    auto &discord = Abaddon::Get().GetDiscordClient();

    Clear();

    // does this function still even work ??lol
    const auto ids = discord.GetUserSortedGuilds();
    for (const auto id : ids) {
        AddGuild(id);
    }
}

void GuildList::AddGuild(Snowflake id) {
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(id);
    if (!guild.has_value()) return;

    auto *item = Gtk::make_managed<GuildListGuildItem>(*guild);
    item->signal_button_press_event().connect([this, id](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            m_signal_guild_selected.emit(id);
        }
        return true;
    });
    item->show();
    add(*item);
}

void GuildList::Clear() {
    const auto children = get_children();
    for (auto *child : children) {
        delete child;
    }
}

GuildList::type_signal_guild_selected GuildList::signal_guild_selected() {
    return m_signal_guild_selected;
}
