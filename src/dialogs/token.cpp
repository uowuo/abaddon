#include "token.hpp"

std::string trim(const std::string &str) {
    const auto first = str.find_first_not_of(' ');
    if (first == std::string::npos) return str;
    const auto last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

TokenDialog::TokenDialog(Gtk::Window &parent)
    : Gtk::Dialog("Set Token", parent, true)
    , m_layout(Gtk::ORIENTATION_VERTICAL)
    , m_ok("OK")
    , m_cancel("Cancel")
    , m_bbox(Gtk::ORIENTATION_HORIZONTAL) {
    set_default_size(300, 50);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    m_ok.signal_clicked().connect([&]() {
        m_token = trim(m_entry.get_text());
        response(Gtk::RESPONSE_OK);
    });

    m_cancel.signal_clicked().connect([&]() {
        response(Gtk::RESPONSE_CANCEL);
    });

    m_bbox.pack_start(m_ok, Gtk::PACK_SHRINK);
    m_bbox.pack_start(m_cancel, Gtk::PACK_SHRINK);
    m_bbox.set_layout(Gtk::BUTTONBOX_END);

    m_entry.set_input_purpose(Gtk::INPUT_PURPOSE_PASSWORD);
    m_entry.set_visibility(false);
    m_entry.set_hexpand(true);
    m_layout.add(m_entry);
    m_layout.add(m_bbox);
    get_content_area()->add(m_layout);

    show_all_children();
}

std::string TokenDialog::GetToken() {
    return m_token;
}
