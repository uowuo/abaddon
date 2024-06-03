#pragma once

#include "discord/snowflake.hpp"

#include <gtkmm/listboxrow.h>

class QuickSwitcherResultRow : public Gtk::ListBoxRow {
public:
    QuickSwitcherResultRow(Snowflake id);
    Snowflake ID;
};
