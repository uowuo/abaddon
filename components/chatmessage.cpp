#include "chatmessage.hpp"

ChatMessageContainer::ChatMessageContainer(const MessageData *data) {
    UserID = data->Author.ID;

    m_main_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_content_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_meta_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_author = Gtk::manage(new Gtk::Label);
    m_timestamp = Gtk::manage(new Gtk::Label);

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

    m_content_box->set_can_focus(false);
    m_content_box->show();

    m_meta_box->add(*m_author);
    m_meta_box->add(*m_timestamp);
    m_content_box->add(*m_meta_box);
    m_main_box->add(*m_content_box);
    add(*m_main_box);
    set_margin_bottom(8);

    show();
}

void ChatMessageContainer::AddNewContent(Gtk::Widget *widget, bool prepend) {
    if (prepend)
        m_content_box->pack_end(*widget);
    else
        m_content_box->pack_start(*widget);
}

ChatMessageTextItem::ChatMessageTextItem(const MessageData *data) {
    set_can_focus(false);
    set_editable(false);
    set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    set_halign(Gtk::ALIGN_FILL);
    set_hexpand(true);
    get_buffer()->set_text(data->Content);
    show();
}

void ChatMessageTextItem::MarkAsDeleted() {
    auto buf = get_buffer();
    Gtk::TextBuffer::iterator start, end;
    buf->get_bounds(start, end);
    buf->insert_markup(end, "<span color='#ff0000'> [deleted]</span>");
}
