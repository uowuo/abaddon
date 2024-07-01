#pragma once

#include "components/lazyimage.hpp"
#include "discord/channel.hpp"
#include "discord/guild.hpp"

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

class QuickSwitcherResultRowChannel : public QuickSwitcherResultRow {
public:
    QuickSwitcherResultRowChannel(const ChannelData &channel);

private:
    Gtk::HBox m_box;
    Gtk::Label m_channel_label;
    Gtk::Label m_guild_label;
};

class QuickSwitcherResultRowGuild : public QuickSwitcherResultRow {
public:
    QuickSwitcherResultRowGuild(const GuildData &guild);

private:
    Gtk::HBox m_box;
    LazyImage m_img;
    Gtk::Label m_label;
};
