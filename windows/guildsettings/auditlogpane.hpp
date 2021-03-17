#pragma once
#include <gtkmm.h>
#include "../../components//inotifyswitched.hpp"
#include "../../discord/objects.hpp"

class GuildSettingsAuditLogPane : public Gtk::ScrolledWindow, public INotifySwitched {
public:
    GuildSettingsAuditLogPane(Snowflake id);

private:
    void on_switched_to() override;

    bool m_requested = false;

    Gtk::ListBox m_list;

    void OnAuditLogFetch(const AuditLogData &data);

    Snowflake GuildID;
};
