#pragma once

#include "components/lazyimage.hpp"
#include "discord/channel.hpp"

#include <gtkmm/hvbox.h>
#include <gtkmm/label.h>
#include <gtkmm/listboxrow.h>

class QuickSwitcherResultRow : public Gtk::ListBoxRow {
public:
    QuickSwitcherResultRow(Snowflake id);
    Snowflake ID;
};

class QuickSwitcherResultRowDM : public QuickSwitcherResultRow {
public:
    QuickSwitcherResultRowDM(const ChannelData &channel);

private:
    Gtk::HBox m_box;
    LazyImage m_img;
    Gtk::Label m_label;
};
