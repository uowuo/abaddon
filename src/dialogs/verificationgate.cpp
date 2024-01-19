#include "verificationgate.hpp"

#include "abaddon.hpp"

VerificationGateDialog::VerificationGateDialog(Gtk::Window &parent, Snowflake guild_id)
    : Gtk::Dialog("Verification Required", parent, true)
    , m_bbox(Gtk::ORIENTATION_HORIZONTAL) {
    set_default_size(300, 300);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    m_ok_button = add_button("Accept", Gtk::RESPONSE_OK);

    m_scroll_rules.set_vexpand(true);
    m_scroll_rules.set_hexpand(true);

    m_description.set_line_wrap(true);
    m_description.set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
    m_description.set_halign(Gtk::ALIGN_CENTER);
    m_description.set_margin_bottom(5);

    m_scroll_rules.add(m_rules);
    get_content_area()->add(m_description);
    get_content_area()->add(m_scroll_rules);
    show_all_children();

    Abaddon::Get().GetDiscordClient().GetVerificationGateInfo(guild_id, sigc::mem_fun(*this, &VerificationGateDialog::OnVerificationGateFetch));
}

const VerificationGateInfoObject &VerificationGateDialog::GetVerificationGate() const {
    return m_gate_info;
}

void VerificationGateDialog::OnVerificationGateFetch(const std::optional<VerificationGateInfoObject> &info) {
    m_gate_info = *info;
    if (m_gate_info.Description.has_value())
        m_description.set_markup("<b>" + Glib::Markup::escape_text(*m_gate_info.Description) + "</b>");
    else
        m_description.hide();
    for (const auto &field : *info->VerificationFields) {
        if (field.Type == "TERMS") {
            for (const auto &rule : field.Values) {
                auto *lbl = Gtk::manage(new Gtk::Label(rule));
                lbl->set_halign(Gtk::ALIGN_START);
                lbl->set_ellipsize(Pango::ELLIPSIZE_END);
                lbl->show();
                m_rules.add(*lbl);
            }
            break;
        }
    }
}
