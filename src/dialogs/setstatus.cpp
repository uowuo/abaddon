#include "setstatus.hpp"

#include <fmt/format.h>
#include <glibmm/i18n.h>

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
    : Gtk::Dialog(_("Set Status"), parent, true)
    , m_layout(Gtk::ORIENTATION_VERTICAL)
    , m_ok("OK")
    , m_cancel(_("Cancel")) {
    set_default_size(350, 200);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");
    get_style_context()->add_class("set-status-dialog");

    m_text.set_placeholder_text("I feel " + Glib::ustring(feelings[rand() % feelings.size()]) + "!");

    m_status_combo.append("online", _("Online"));
    m_status_combo.append("dnd", _("Do Not Disturb"));
    m_status_combo.append("idle", _("Away"));
    m_status_combo.append("invisible", _("Invisible"));
    m_status_combo.set_active_text(_("Online"));

    m_type_combo.append("0", _("Playing"));
    m_type_combo.append("1", _("Streaming"));
    m_type_combo.append("2", _("Listening to"));
    m_type_combo.append("3", _("Watching"));
    m_type_combo.append("4", _("Custom"));
    m_type_combo.append("5", _("Competing in"));
    m_type_combo.set_active_text(_("Custom"));

    m_ok.signal_clicked().connect([this]() {
        response(Gtk::RESPONSE_OK);
    });

    m_cancel.signal_clicked().connect([this]() {
        response(Gtk::RESPONSE_CANCEL);
    });

    m_layout.pack_start(*Gtk::make_managed<Gtk::Label>(fmt::format(_("How are you, {}?"), Abaddon::Get().GetDiscordClient().GetUserData().GetDisplayName().c_str()), Gtk::ALIGN_START));
    m_layout.pack_start(m_text);
    m_layout.pack_start(*Gtk::make_managed<Gtk::Label>(_("Status"), Gtk::ALIGN_START));
    m_layout.pack_start(m_status_combo);
    m_layout.pack_start(*Gtk::make_managed<Gtk::Label>(_("Activity"), Gtk::ALIGN_START));
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
