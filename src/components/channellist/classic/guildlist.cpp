#include "guildlist.hpp"
#include "guildlistfolderitem.hpp"

GuildList::GuildList() {
    set_selection_mode(Gtk::SELECTION_NONE);
    show_all_children();
}

void GuildList::AddGuild(Snowflake id) {
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(id);
    if (!guild.has_value()) return;

    auto *item = Gtk::make_managed<GuildListGuildItem>(*guild);
    item->show();
    add(*item);
}

void GuildList::Clear() {
    const auto children = get_children();
    for (auto child : children) {
        delete child;
    }
}
