#pragma once
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include "discord/guild.hpp"

class GuildListGuildItem : public Gtk::EventBox {
public:
    GuildListGuildItem(const GuildData &guild);

    Snowflake ID;

private:
    void UpdateIcon();
    void OnIconFetched(const Glib::RefPtr<Gdk::Pixbuf> &pb);

    Gtk::Image m_image;
};
