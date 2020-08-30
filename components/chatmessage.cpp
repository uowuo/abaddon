#include "chatmessage.hpp"
#include "../abaddon.hpp"

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

ChatMessageItem::ChatMessageItem() {
    m_menu_copy_id = Gtk::manage(new Gtk::MenuItem("_Copy ID", true));
    m_menu_copy_id->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageItem::on_menu_copy_id));
    m_menu.append(*m_menu_copy_id);

    m_menu_delete_message = Gtk::manage(new Gtk::MenuItem("_Delete Message", true));
    m_menu_delete_message->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageItem::on_menu_message_delete));
    m_menu.append(*m_menu_delete_message);

    m_menu.show_all();
}

void ChatMessageItem::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
}

void ChatMessageItem::on_menu_message_delete() {
    m_abaddon->ActionChatDeleteMessage(ChannelID, ID);
}

void ChatMessageItem::on_menu_copy_id() {
    Gtk::Clipboard::get()->set_text(std::to_string(ID));
}

// broken format v
// clang-format off
void ChatMessageItem::AttachMenuHandler(Gtk::Widget *widget) {
    widget->signal_button_press_event().connect([this](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
            ShowMenu(reinterpret_cast<GdkEvent *>(event));
            return true;
        }

        return false;
    }, false);
}
// clang-format on

void ChatMessageItem::ShowMenu(const GdkEvent *event) {
    auto &client = m_abaddon->GetDiscordClient();
    auto *data = client.GetMessage(ID);
    m_menu_delete_message->set_sensitive(client.GetUserData().ID == data->Author.ID);
    m_menu.popup_at_pointer(event);
}

void ChatMessageItem::AddMenuItem(Gtk::MenuItem *item) {
    item->show();
    m_menu.append(*item);
}

ChatMessageTextItem::ChatMessageTextItem(const MessageData *data) {
    set_can_focus(false);
    set_editable(false);
    set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    set_halign(Gtk::ALIGN_FILL);
    set_hexpand(true);
    get_buffer()->set_text(data->Content);
    show();

    AttachMenuHandler(this);
    m_menu_copy_content = Gtk::manage(new Gtk::MenuItem("Copy _Message", true));
    AddMenuItem(m_menu_copy_content);
    m_menu_copy_content->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageTextItem::on_menu_copy_content));
}

void ChatMessageTextItem::on_menu_copy_content() {
    auto *data = m_abaddon->GetDiscordClient().GetMessage(ID);
    Gtk::Clipboard::get()->set_text(data->Content);
}

void ChatMessageTextItem::MarkAsDeleted() {
    auto buf = get_buffer();
    Gtk::TextBuffer::iterator start, end;
    buf->get_bounds(start, end);
    buf->insert_markup(end, "<span color='#ff0000'> [deleted]</span>");
}
