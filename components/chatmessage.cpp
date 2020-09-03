#include <sstream>
#include <iomanip>
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

    m_menu_edit_message = Gtk::manage(new Gtk::MenuItem("_Edit Message", true));
    m_menu_edit_message->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageItem::on_menu_message_edit));
    m_menu.append(*m_menu_edit_message);

    m_menu.show_all();
}

void ChatMessageItem::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
}

void ChatMessageItem::on_menu_message_delete() {
    m_abaddon->ActionChatDeleteMessage(ChannelID, ID);
}

void ChatMessageItem::on_menu_message_edit() {
    m_abaddon->ActionChatEditMessage(ChannelID, ID);
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
    bool can_manage = client.GetUserData().ID == data->Author.ID;
    m_menu_delete_message->set_sensitive(can_manage);
    m_menu_edit_message->set_sensitive(can_manage);
    m_menu.popup_at_pointer(event);
}

void ChatMessageItem::AddMenuItem(Gtk::MenuItem *item) {
    item->show();
    m_menu.append(*item);
}

ChatMessageTextItem::ChatMessageTextItem(const MessageData *data) {
    m_content = data->Content;

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

void ChatMessageTextItem::EditContent(std::string content) {
    m_content = content;
    get_buffer()->set_text(content);
    UpdateAttributes();
}

void ChatMessageTextItem::on_menu_copy_content() {
    Gtk::Clipboard::get()->set_text(m_content);
}

void ChatMessageTextItem::MarkAsDeleted() {
    m_was_deleted = true;
    UpdateAttributes();
}

void ChatMessageTextItem::MarkAsEdited() {
    m_was_edited = true;
    UpdateAttributes();
}

void ChatMessageTextItem::UpdateAttributes() {
    bool deleted = m_was_deleted;
    bool edited = m_was_edited && !m_was_deleted;

    auto buf = get_buffer();
    buf->set_text(m_content);
    Gtk::TextBuffer::iterator start, end;
    buf->get_bounds(start, end);

    if (deleted) {
        buf->insert_markup(end, "<span color='#ff0000'> [deleted]</span>");
    } else if (edited) {
        buf->insert_markup(end, "<span color='#999999'> [edited]</span>");
    }
}

ChatMessageEmbedItem::ChatMessageEmbedItem(const MessageData *data) {
    m_embed = data->Embeds[0];

    DoLayout();
    AttachMenuHandler(this);
}

void ChatMessageEmbedItem::DoLayout() {
    m_main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

    auto children = get_children();
    auto it = children.begin();

    while (it != children.end()) {
        delete *it;
        it++;
    }

    if (m_embed.Title.length() > 0) {
        auto *title_label = Gtk::manage(new Gtk::Label);
        title_label->set_use_markup(true);
        title_label->set_markup("<b>" + Glib::Markup::escape_text(m_embed.Title) + "</b>");
        title_label->set_halign(Gtk::ALIGN_CENTER);
        title_label->set_hexpand(false);
        m_main->pack_start(*title_label);
    }

    if (m_embed.Description.length() > 0) {
        auto *desc_label = Gtk::manage(new Gtk::Label);
        desc_label->set_text(m_embed.Description);
        desc_label->set_line_wrap(true);
        desc_label->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        desc_label->set_max_width_chars(50);
        desc_label->set_halign(Gtk::ALIGN_START);
        desc_label->set_hexpand(false);
        m_main->pack_start(*desc_label);
    }

    if (m_embed.Fields.size() > 0) {
        auto *flow = Gtk::manage(new Gtk::FlowBox);
        flow->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        flow->set_min_children_per_line(3);
        flow->set_halign(Gtk::ALIGN_START);
        flow->set_hexpand(false);
        flow->set_column_spacing(10);
        m_main->add(*flow);

        for (const auto &field : m_embed.Fields) {
            auto *field_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
            auto *field_lbl = Gtk::manage(new Gtk::Label);
            auto *field_val = Gtk::manage(new Gtk::Label);
            field_box->set_hexpand(false);
            field_box->set_halign(Gtk::ALIGN_START);
            field_lbl->set_hexpand(false);
            field_lbl->set_halign(Gtk::ALIGN_START);
            field_val->set_hexpand(false);
            field_val->set_halign(Gtk::ALIGN_START);
            field_lbl->set_use_markup(true);
            field_lbl->set_markup("<b>" + Glib::Markup::escape_text(field.Name) + "</b>");
            field_lbl->set_max_width_chars(20);
            field_lbl->set_line_wrap(true);
            field_lbl->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
            field_val->set_text(field.Value);
            field_val->set_max_width_chars(20);
            field_val->set_line_wrap(true);
            field_val->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
            field_box->pack_start(*field_lbl);
            field_box->pack_start(*field_val);
            flow->insert(*field_box, -1);
        }
    }

    auto style = m_main->get_style_context();

    if (m_embed.Color != -1) {
        auto provider = Gtk::CssProvider::create(); // this seems wrong
        int r = (m_embed.Color & 0xFF0000) >> 16;
        int g = (m_embed.Color & 0x00FF00) >> 8;
        int b = (m_embed.Color & 0x0000FF) >> 0;
        std::stringstream css; // lol
        css << ".embed { border-left: 2px solid #"
            << std::hex << std::setw(2) << std::setfill('0') << r
            << std::hex << std::setw(2) << std::setfill('0') << g
            << std::hex << std::setw(2) << std::setfill('0') << b
            << "; }";

        provider->load_from_data(css.str());
        style->add_provider(provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    style->add_class("embed");

    set_margin_bottom(8);
    set_hexpand(false);
    m_main->set_hexpand(false);
    m_main->set_halign(Gtk::ALIGN_START);
    set_halign(Gtk::ALIGN_START);

    add(*m_main);
    show_all();
}

void ChatMessageEmbedItem::UpdateAttributes() {
    bool deleted = m_was_deleted;
    bool edited = m_was_edited && !m_was_deleted;

    if (m_attrib_label == nullptr) {
        m_attrib_label = Gtk::manage(new Gtk::Label);
        m_attrib_label->set_use_markup(true);
        m_attrib_label->show();
        m_main->pack_end(*m_attrib_label);
    }

    if (deleted)
        m_attrib_label->set_markup(" <span color='#ff0000'> [deleted]</span>");
    else if (edited)
        m_attrib_label->set_markup(" <span color='#999999'> [edited]</span>");
}

void ChatMessageEmbedItem::MarkAsDeleted() {
    m_was_deleted = true;
    // UpdateAttributes(); untested
}

void ChatMessageEmbedItem::MarkAsEdited() {
    m_was_edited = true;
    // UpdateAttributes();
}
