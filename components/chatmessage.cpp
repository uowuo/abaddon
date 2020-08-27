#include "chatmessage.hpp"

ChatMessageTextItem::ChatMessageTextItem(const MessageData *data) {
    m_main_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_sub_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_meta_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_author = Gtk::manage(new Gtk::Label);
    m_timestamp = Gtk::manage(new Gtk::Label);
    m_text = Gtk::manage(new Gtk::TextView);

    m_text->set_can_focus(false);
    m_text->set_editable(false);
    m_text->set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    m_text->set_halign(Gtk::ALIGN_FILL);
    m_text->set_hexpand(true);
    m_text->get_buffer()->set_text(data->Content);
    m_text->show();

    m_author->set_markup("<span weight=\"bold\">" + Glib::Markup::escape_text(data->Author.Username) + "</span>");
    m_author->set_single_line_mode(true);
    m_author->set_line_wrap(false);
    m_author->set_ellipsize(Pango::ELLIPSIZE_END);
    m_author->set_xalign(0.f);
    m_author->set_can_focus(false);
    m_author->show();

    m_timestamp->set_text(data->Timestamp);
    m_timestamp->set_opacity(0.5);
    m_timestamp->set_single_line_mode(true);
    m_timestamp->set_margin_start(12);
    m_timestamp->set_can_focus(false);
    m_timestamp->show();

    m_main_box->set_hexpand(true);
    m_main_box->set_vexpand(true);
    m_main_box->set_can_focus(true);
    m_main_box->show();

    m_meta_box->set_can_focus(false);
    m_meta_box->show();

    m_sub_box->set_can_focus(false);
    m_sub_box->show();

    m_meta_box->add(*m_author);
    m_meta_box->add(*m_timestamp);
    m_sub_box->add(*m_meta_box);
    m_sub_box->add(*m_text);
    m_main_box->add(*m_sub_box);
    add(*m_main_box);
    set_margin_bottom(8);

    show();
}

void ChatMessageTextItem::AppendNewContent(std::string content) {
    auto buf = m_text->get_buffer();
    buf->set_text(buf->get_text() + "\n" + content);
}
