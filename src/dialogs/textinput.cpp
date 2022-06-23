#include "textinput.hpp"

TextInputDialog::TextInputDialog(const Glib::ustring &prompt, const Glib::ustring &title, const Glib::ustring &placeholder, Gtk::Window &parent)
    : Gtk::Dialog(title, parent, true)
    , m_label(prompt) {
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    auto ok = add_button("OK", Gtk::RESPONSE_OK);
    auto cancel = add_button("Cancel", Gtk::RESPONSE_CANCEL);

    get_content_area()->add(m_label);
    get_content_area()->add(m_entry);

    m_entry.set_text(placeholder);

    m_entry.set_activates_default(true);
    ok->set_can_default(true);
    ok->grab_default();

    show_all_children();
}

Glib::ustring TextInputDialog::GetInput() const {
    return m_entry.get_text();
}
