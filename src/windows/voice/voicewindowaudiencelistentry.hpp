#pragma once

#include "components/lazyimage.hpp"
#include "discord/snowflake.hpp"

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/listboxrow.h>

class VoiceWindowAudienceListEntry : public Gtk::ListBoxRow {
public:
    VoiceWindowAudienceListEntry(Snowflake id);

private:
    Gtk::Box m_main;
    LazyImage m_avatar;
    Gtk::Label m_name;
};
