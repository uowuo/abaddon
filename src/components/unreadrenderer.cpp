#include "unreadrenderer.hpp"
#include "abaddon.hpp"

void UnreadRenderer::RenderUnreadOnChannel(Snowflake id, const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area) {
    const auto state = Abaddon::Get().GetDiscordClient().GetUnreadStateForChannel(id);
    if (state >= 0) {
        cr->set_source_rgb(1.0, 1.0, 1.0);
        const auto x = cell_area.get_x() + 1;
        const auto y = cell_area.get_y();
        const auto w = cell_area.get_width();
        const auto h = cell_area.get_height();
        cr->rectangle(x, y, 3, h);
        cr->fill();
    }
}
