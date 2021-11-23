#include "setstatus.hpp"

SetStatusDialog::SetStatusDialog(Gtk::Window &parent)
    : Gtk::Dialog("Set Status", parent, true)
    , m_layout(Gtk::ORIENTATION_VERTICAL)
    , m_bottom(Gtk::ORIENTATION_HORIZONTAL)
    , m_ok("OK")
    , m_cancel("Cancel")
    , m_bbox(Gtk::ORIENTATION_HORIZONTAL) {
    set_default_size(300, 50);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    m_text.set_placeholder_text("Status text");

    m_status_combo.append("online", "Online");
    m_status_combo.append("dnd", "Do Not Disturb");
    m_status_combo.append("idle", "Away");
    m_status_combo.append("invisible", "Invisible");
    m_status_combo.set_active_text("Online");

    m_type_combo.append("0", "Playing");
    m_type_combo.append("1", "Streaming");
    m_type_combo.append("2", "Listening to");
    m_type_combo.append("3", "Watching");
    m_type_combo.append("4", "Custom");
    m_type_combo.append("5", "Competing in");
    m_type_combo.set_active_text("Custom");

    m_ok.signal_clicked().connect([this]() {
        response(Gtk::RESPONSE_OK);
    });

    m_cancel.signal_clicked().connect([this]() {
        response(Gtk::RESPONSE_CANCEL);
    });

    m_bbox.pack_start(m_ok, Gtk::PACK_SHRINK);
    m_bbox.pack_start(m_cancel, Gtk::PACK_SHRINK);
    m_bbox.set_layout(Gtk::BUTTONBOX_END);

    m_bottom.add(m_status_combo);
    m_bottom.add(m_type_combo);
    m_bottom.add(m_bbox);
    m_layout.add(m_text);
    m_layout.add(m_bottom);
    get_content_area()->add(m_layout);

    show_all_children();
}

ActivityType SetStatusDialog::GetActivityType() const {
    const auto x = m_type_combo.get_active_id();
    return static_cast<ActivityType>(std::stoul(x));
}

PresenceStatus SetStatusDialog::GetStatusType() const {
    const auto &x = m_status_combo.get_active_id();
    if (x == "online")
        return PresenceStatus::Online;
    else if (x == "idle")
        return PresenceStatus::Idle;
    else if (x == "dnd")
        return PresenceStatus::DND;
    else if (x == "invisible")
        return PresenceStatus::Offline;
    return PresenceStatus::Online;
}

std::string SetStatusDialog::GetActivityName() const {
    return m_text.get_text();
}
