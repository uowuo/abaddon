#include "unreadrenderer.hpp"
#include "abaddon.hpp"

constexpr static int MentionsRightPad = 7;
constexpr static double M_PI = 3.14159265358979;
constexpr static double M_PI_H = M_PI / 2.0;
constexpr static double M_PI_3_2 = M_PI * 3.0 / 2.0;

void CairoPathRoundedRect(const Cairo::RefPtr<Cairo::Context> &cr, double x, double y, double w, double h, double r) {
    const double degrees = M_PI / 180.0;

    cr->begin_new_sub_path();
    cr->arc(x + w - r, y + r, r, -M_PI_H, 0);
    cr->arc(x + w - r, y + h - r, r, 0, M_PI_H);
    cr->arc(x + r, y + h - r, r, M_PI_H, M_PI);
    cr->arc(x + r, y + r, r, M_PI, M_PI_3_2);
    cr->close_path();
}

void RenderMentionsCount(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, int mentions, int edge, const Gdk::Rectangle &cell_area) {
    Pango::FontDescription font;
    font.set_family("sans 14");
    //font.set_weight(Pango::WEIGHT_BOLD);

    auto layout = widget.create_pango_layout(std::to_string(mentions));
    layout->set_font_description(font);
    layout->set_alignment(Pango::ALIGN_RIGHT);

    int width, height;
    layout->get_pixel_size(width, height);
    {
        const auto x = cell_area.get_x() + edge - width - MentionsRightPad;
        const auto y = cell_area.get_y() + cell_area.get_height() / 2.0 - height / 2.0;
        CairoPathRoundedRect(cr, x - 4, y + 2, width + 8, height, 5);
        cr->set_source_rgb(184.0 / 255.0, 37.0 / 255.0, 37.0 / 255.0);
        cr->fill();
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->move_to(x, y);
        layout->show_in_cairo_context(cr);
    }
}

void UnreadRenderer::RenderUnreadOnGuild(Snowflake id, Gtk::Widget &widget, const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area) {
    // maybe have DiscordClient track this?
    int total_mentions = 0;
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto channels = discord.GetChannelsInGuild(id);
    bool has_unread = false;
    for (const auto &id : channels) {
        const int state = Abaddon::Get().GetDiscordClient().GetUnreadStateForChannel(id);
        if (state >= 0) {
            has_unread = true;
            total_mentions += state;
        }
    }
    if (!has_unread) return;

    if (!discord.IsGuildMuted(id)) {
        cr->set_source_rgb(1.0, 1.0, 1.0);
        const auto x = background_area.get_x();
        const auto y = background_area.get_y();
        const auto w = background_area.get_width();
        const auto h = background_area.get_height();
        cr->rectangle(x, y + h / 2 - 24 / 2, 3, 24);
        cr->fill();
    }

    if (total_mentions < 1) return;
    auto *paned = static_cast<Gtk::Paned *>(widget.get_ancestor(Gtk::Paned::get_type()));
    if (paned != nullptr) {
        const auto edge = std::min(paned->get_position(), cell_area.get_width());

        RenderMentionsCount(cr, widget, total_mentions, edge, cell_area);
    }
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
    if (paned != nullptr) {
        const auto edge = std::min(paned->get_position(), cell_area.get_width());

        RenderMentionsCount(cr, widget, state, edge, cell_area);
    }
}
