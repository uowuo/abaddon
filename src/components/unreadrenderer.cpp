#include "unreadrenderer.hpp"
#include "abaddon.hpp"

void UnreadRenderer::RenderUnreadOnGuild(Snowflake id, Gtk::Widget &widget, const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area) {
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

void UnreadRenderer::RenderUnreadOnChannel(Snowflake id, Gtk::Widget &widget, const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area) {
    const auto state = Abaddon::Get().GetDiscordClient().GetUnreadStateForChannel(id);
    if (state < 0) return;
    cr->set_source_rgb(1.0, 1.0, 1.0);
    const auto x = background_area.get_x();
    const auto y = background_area.get_y();
    const auto w = background_area.get_width();
    const auto h = background_area.get_height();
    cr->rectangle(x, y, 3, h);
    cr->fill();

    if (state < 1) return;
    auto *paned = static_cast<Gtk::Paned *>(widget.get_ancestor(Gtk::Paned::get_type()));
    const auto edge = paned->get_position();

    // surely this shouldnt be run every draw?
    // https://developer-old.gnome.org/gtkmm-tutorial/3.22/sec-drawing-text.html.en

    Pango::FontDescription font;
    font.set_family("sans 14");
    font.set_weight(Pango::WEIGHT_BOLD);

    auto layout = widget.create_pango_layout(std::to_string(state));
    layout->set_font_description(font);
    layout->set_alignment(Pango::ALIGN_RIGHT);

    int width, height;
    layout->get_pixel_size(width, height);
    {
        const auto x = cell_area.get_x() + std::min(edge, cell_area.get_width()) - 14 - 2;
        const auto y = cell_area.get_y() + cell_area.get_height() / 2.0 - height / 2.0;
        cr->move_to(x, y);
        layout->show_in_cairo_context(cr);
    }
}
