#pragma once

#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>

#include "discord/objects.hpp"

class GuildSettingsAuditLogPane : public Gtk::ScrolledWindow {
public:
    GuildSettingsAuditLogPane(Snowflake id);

private:
    void OnMap();

    bool m_requested = false;

    Gtk::ListBox m_list;

    void OnAuditLogFetch(const AuditLogData &data);

    Snowflake GuildID;
};
