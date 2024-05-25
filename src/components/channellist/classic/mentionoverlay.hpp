#pragma once

#include <gtkmm/drawingarea.h>
#include <pangomm/fontdescription.h>

#include "discord/snowflake.hpp"
#include "discord/usersettings.hpp"

class MentionOverlay : public Gtk::DrawingArea {
public:
    MentionOverlay(Snowflake guild_id);
    MentionOverlay(const UserSettingsGuildFoldersEntry &folder);

private:
    void Init();

    bool OnDraw(const Cairo::RefPtr<Cairo::Context> &cr);

    std::set<Snowflake> m_guild_ids;

    Pango::FontDescription m_font;
    Glib::RefPtr<Pango::Layout> m_layout;
};
