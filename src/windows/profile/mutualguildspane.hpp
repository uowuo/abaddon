#pragma once

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>

#include "discord/objects.hpp"

class MutualGuildItem : public Gtk::Box {
public:
    MutualGuildItem(const MutualGuildData &guild);

private:
    Gtk::Image m_icon;
    Gtk::Box m_box;
    Gtk::Label m_name;
    Gtk::Label *m_nick = nullptr;
};

class UserMutualGuildsPane : public Gtk::ScrolledWindow {
public:
    UserMutualGuildsPane(Snowflake id);

    void SetMutualGuilds(const std::vector<MutualGuildData> &guilds);

    Snowflake UserID;

private:
    Gtk::ListBox m_list;
};
