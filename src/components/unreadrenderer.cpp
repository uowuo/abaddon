#include "unreadrenderer.hpp"
#include "abaddon.hpp"

void UnreadRenderer::RenderUnreadOnGuild(Snowflake id, const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area) {
    // maybe have DiscordClient track this?
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto channels = discord.GetChannelsInGuild(id);
    bool has_unread = false;
    for (const auto &id : channels) {
        if (Abaddon::Get().GetDiscordClient().GetUnreadStateForChannel(id) >= 0) {
            has_unread = true;
            break;
        }
    }
    if (!has_unread) return;

    cr->set_source_rgb(1.0, 1.0, 1.0);
    const auto x = background_area.get_x();
    const auto y = background_area.get_y();
    const auto w = background_area.get_width();
    const auto h = background_area.get_height();
    cr->rectangle(x, y + h / 2 - 24 / 2, 3, 24);
    cr->fill();
}

void UnreadRenderer::RenderUnreadOnChannel(Snowflake id, const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area) {
    const auto state = Abaddon::Get().GetDiscordClient().GetUnreadStateForChannel(id);
    if (state >= 0) {
        cr->set_source_rgb(1.0, 1.0, 1.0);
        const auto x = background_area.get_x();
        const auto y = background_area.get_y();
        const auto w = background_area.get_width();
        const auto h = background_area.get_height();
        cr->rectangle(x, y, 3, h);
        cr->fill();
    }
}
