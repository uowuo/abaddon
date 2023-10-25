#pragma once
#include <gtkmm/listbox.h>
#include "discord/snowflake.hpp"

class GuildList : public Gtk::ListBox {
public:
    GuildList();

    void UpdateListing();

private:
    void AddGuild(Snowflake id);
    void Clear();

public:
    using type_signal_guild_selected = sigc::signal<void, Snowflake>;

    type_signal_guild_selected signal_guild_selected();

private:
    type_signal_guild_selected m_signal_guild_selected;
};
