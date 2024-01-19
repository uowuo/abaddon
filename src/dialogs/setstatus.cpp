#include "setstatus.hpp"

#include "abaddon.hpp"

static const std::array feelings = {
    "wonderful",
    "splendiferous",
    "delicious",
    "outstanding",
    "amazing",
    "great",
    "marvelous",
    "superb",
    "out of this world",
    "stupendous",
    "tip-top",
    "horrible",
};

SetStatusDialog::SetStatusDialog(Gtk::Window &parent)
    : Gtk::Dialog("Set Status", parent, true)
    , m_layout(Gtk::ORIENTATION_VERTICAL)
    , m_ok("OK")
    , m_cancel("Cancel") {
    set_default_size(350, 200);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");
    get_style_context()->add_class("set-status-dialog");

    m_text.set_placeholder_text("I feel " + Glib::ustring(feelings[rand() % feelings.size()]) + "!");

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

    m_layout.pack_start(*Gtk::make_managed<Gtk::Label>("How are you, " + Abaddon::Get().GetDiscordClient().GetUserData().GetDisplayName() + "?", Gtk::ALIGN_START));
    m_layout.pack_start(m_text);
    m_layout.pack_start(*Gtk::make_managed<Gtk::Label>("Status", Gtk::ALIGN_START));
    m_layout.pack_start(m_status_combo);
    m_layout.pack_start(*Gtk::make_managed<Gtk::Label>("Activity", Gtk::ALIGN_START));
    m_layout.pack_start(m_type_combo);

    get_content_area()->add(m_layout);
    get_action_area()->pack_start(m_ok, Gtk::PACK_SHRINK);
    get_action_area()->pack_start(m_cancel, Gtk::PACK_SHRINK);
    get_action_area()->set_layout(Gtk::BUTTONBOX_START);

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
