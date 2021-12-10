#pragma once
#include <cairomm/context.h>
#include <gdkmm/rectangle.h>
#include <gtkmm/widget.h>
#include "discord/snowflake.hpp"

class UnreadRenderer {
public:
    static void RenderUnreadOnGuild(Snowflake id, Gtk::Widget &widget, const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area);
    static void RenderUnreadOnChannel(Snowflake id, Gtk::Widget &widget, const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area);
};
