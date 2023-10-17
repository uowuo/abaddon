#pragma once
#include <gtkmm/listbox.h>
#include "discord/snowflake.hpp"

class GuildList : public Gtk::ListBox {
public:
    GuildList();

    void AddGuild(Snowflake id);

    void Clear();
};
