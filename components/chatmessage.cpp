#include "chatmessage.hpp"

ChatMessageTextItem::ChatMessageTextItem(const MessageData *data) {
    auto *main_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    auto *sub_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    auto *meta_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    auto *author = Gtk::manage(new Gtk::Label);
    auto *timestamp = Gtk::manage(new Gtk::Label);
    auto *text = Gtk::manage(new Gtk::TextView);

    text->set_can_focus(false);
    text->set_editable(false);
    text->set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    text->set_halign(Gtk::ALIGN_FILL);
    text->set_hexpand(true);
    text->get_buffer()->set_text(data->Content);
    text->show();

    author->set_markup("<span weight=\"bold\">" + Glib::Markup::escape_text(data->Author.Username) + "</span>");
    author->set_single_line_mode(true);
    author->set_line_wrap(false);
    author->set_ellipsize(Pango::ELLIPSIZE_END);
    author->set_xalign(0.f);
    author->show();

    timestamp->set_text(data->Timestamp);
    timestamp->set_opacity(0.5);
    timestamp->set_single_line_mode(true);
    timestamp->set_margin_start(12);
    timestamp->show();

    main_box->set_hexpand(true);
    main_box->set_vexpand(true);
    main_box->show();

    meta_box->show();
    sub_box->show();

    meta_box->add(*author);
    meta_box->add(*timestamp);
    sub_box->add(*meta_box);
    sub_box->add(*text);
    main_box->add(*sub_box);
    add(*main_box);
    set_margin_bottom(8);

    show();
}
