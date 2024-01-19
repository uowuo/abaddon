#pragma once

#include <gdkmm/pixbuf.h>
#include <gtkmm/entry.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

#include "discord/guild.hpp"

class GuildSettingsInfoPane : public Gtk::Grid {
public:
    GuildSettingsInfoPane(Snowflake id);

private:
    void FetchGuildIcon(const GuildData &guild);

    void UpdateGuildName();
    void UpdateGuildIconFromData(const std::vector<uint8_t> &data, const std::string &mime);
    void UpdateGuildIconFromPixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf);
    void UpdateGuildIconPicker();

    Gtk::EventBox m_guild_icon_ev; // necessary to make custom cursor behave properly
    Gtk::Image m_guild_icon;

    Gtk::Label m_guild_name_label;
    Gtk::Entry m_guild_name;

    Snowflake GuildID;
};
