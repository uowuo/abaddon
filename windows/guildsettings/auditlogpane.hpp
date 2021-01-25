#pragma once
#include <gtkmm.h>
#include "../../discord/objects.hpp"

class GuildSettingsAuditLogPane : public Gtk::ScrolledWindow {
public:
    GuildSettingsAuditLogPane(Snowflake id);

private:
    Gtk::ListBox m_list;

    void OnAuditLogFetch(const AuditLogData &data);

    Snowflake GuildID;
};
