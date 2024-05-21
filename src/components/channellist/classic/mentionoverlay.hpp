#pragma once

#include <gtkmm/drawingarea.h>
#include <pangomm/fontdescription.h>

#include "discord/snowflake.hpp"

class MentionOverlay : public Gtk::DrawingArea {
public:
    MentionOverlay(Snowflake guild_id);

private:
    bool OnDraw(const Cairo::RefPtr<Cairo::Context> &cr);

    Snowflake m_guild_id;

    Pango::FontDescription m_font;
    Glib::RefPtr<Pango::Layout> m_layout;
};
