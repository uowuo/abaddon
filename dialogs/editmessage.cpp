#include "editmessage.hpp"

EditMessageDialog::EditMessageDialog(Gtk::Window &parent)
    : Gtk::Dialog("Edit Message", parent, true)
    , m_layout(Gtk::ORIENTATION_VERTICAL)
    , m_ok("OK")
    , m_cancel("Cancel")
    , m_bbox(Gtk::ORIENTATION_HORIZONTAL) {
    set_default_size(300, 50);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    m_ok.signal_clicked().connect([&]() {
        m_content = m_text.get_buffer()->get_text();
        response(Gtk::RESPONSE_OK);
    });

    m_cancel.signal_clicked().connect([&]() {
        response(Gtk::RESPONSE_CANCEL);
    });

    m_bbox.pack_start(m_ok, Gtk::PACK_SHRINK);
    m_bbox.pack_start(m_cancel, Gtk::PACK_SHRINK);
    m_bbox.set_layout(Gtk::BUTTONBOX_END);

    m_text.set_hexpand(true);

    m_scroll.set_hexpand(true);
    m_scroll.set_vexpand(true);
    m_scroll.add(m_text);

    m_layout.add(m_scroll);
    m_layout.add(m_bbox);
    get_content_area()->add(m_layout);

    show_all_children();
}

Glib::ustring EditMessageDialog::GetContent() {
    return m_content;
}

void EditMessageDialog::SetContent(const Glib::ustring &str) {
    m_text.get_buffer()->set_text(str);
}
