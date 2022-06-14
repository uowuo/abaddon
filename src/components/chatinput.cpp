#include "chatinput.hpp"
#include "abaddon.hpp"
#include "constants.hpp"
#include <filesystem>
#include <utility>

ChatInputText::ChatInputText() {
    get_style_context()->add_class("message-input");
    set_propagate_natural_height(true);
    set_min_content_height(20);
    set_max_content_height(250);
    set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    // hack
    auto cb = [this](GdkEventKey *e) -> bool {
        return event(reinterpret_cast<GdkEvent *>(e));
    };
    m_textview.signal_key_press_event().connect(cb, false);
    m_textview.set_hexpand(false);
    m_textview.set_halign(Gtk::ALIGN_FILL);
    m_textview.set_valign(Gtk::ALIGN_CENTER);
    m_textview.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    m_textview.show();
    add(m_textview);
}

void ChatInputText::InsertText(const Glib::ustring &text) {
    GetBuffer()->insert_at_cursor(text);
    m_textview.grab_focus();
}

Glib::RefPtr<Gtk::TextBuffer> ChatInputText::GetBuffer() {
    return m_textview.get_buffer();
}

// this isnt connected directly so that the chat window can handle stuff like the completer first
bool ChatInputText::ProcessKeyPress(GdkEventKey *event) {
    if (event->keyval == GDK_KEY_Escape) {
        m_signal_escape.emit();
        return true;
    }

    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_v) {
        return CheckHandleClipboardPaste();
    }

    if (event->keyval == GDK_KEY_Return) {
        if (event->state & GDK_SHIFT_MASK)
            return false;

        auto buf = GetBuffer();
        auto text = buf->get_text();

        const bool accepted = m_signal_submit.emit(text);
        if (accepted)
            buf->set_text("");

        return true;
    }

    return false;
}

void ChatInputText::on_grab_focus() {
    m_textview.grab_focus();
}

bool ChatInputText::CheckHandleClipboardPaste() {
    auto clip = Gtk::Clipboard::get();
    if (!clip->wait_is_image_available()) return false;

    const auto pb = clip->wait_for_image();
    if (pb) {
        m_signal_image_paste.emit(pb);

        return true;
    } else {
        return false;
    }
}

ChatInputText::type_signal_submit ChatInputText::signal_submit() {
    return m_signal_submit;
}

ChatInputText::type_signal_escape ChatInputText::signal_escape() {
    return m_signal_escape;
}

ChatInputText::type_signal_image_paste ChatInputText::signal_image_paste() {
    return m_signal_image_paste;
}

ChatInputAttachmentContainer::ChatInputAttachmentContainer()
    : m_box(Gtk::ORIENTATION_HORIZONTAL) {
    get_style_context()->add_class("attachment-container");

    add(m_box);
    m_box.show();

    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);
    set_vexpand(true);
    set_size_request(-1, AttachmentItemSize + 10);
}

void ChatInputAttachmentContainer::Clear() {
    for (auto *item : m_attachments) {
        std::error_code ec;
        std::filesystem::remove(item->GetPath(), ec);
        delete item;
    }
    m_attachments.clear();
}

void ChatInputAttachmentContainer::ClearNoPurge() {
    for (auto *item : m_attachments) {
        delete item;
    }
    m_attachments.clear();
}

bool ChatInputAttachmentContainer::AddImage(const Glib::RefPtr<Gdk::Pixbuf> &pb) {
    if (m_attachments.size() == 10) return false;

    static unsigned go_up = 0;
    std::string dest_name = "pasted-image-" + std::to_string(go_up++);
    const auto path = (std::filesystem::temp_directory_path() / "abaddon-cache" / dest_name).string();

    try {
        pb->save(path, "png");
    } catch (...) {
        fprintf(stderr, "pasted image save error\n");
        return false;
    }

    auto *item = Gtk::make_managed<ChatInputAttachmentItem>(path, pb);
    item->show();
    item->set_valign(Gtk::ALIGN_CENTER);
    m_box.add(*item);

    m_attachments.insert(item);

    item->signal_remove().connect([this, item] {
        std::error_code ec;
        std::filesystem::remove(item->GetPath(), ec);
        m_attachments.erase(item);
        delete item;
        if (m_attachments.empty())
            m_signal_emptied.emit();
    });

    return true;
}

std::vector<ChatSubmitParams::Attachment> ChatInputAttachmentContainer::GetAttachments() const {
    std::vector<ChatSubmitParams::Attachment> ret;
    for (auto *x : m_attachments)
        ret.push_back({ x->GetPath(), x->GetType() });
    return ret;
}

ChatInputAttachmentContainer::type_signal_emptied ChatInputAttachmentContainer::signal_emptied() {
    return m_signal_emptied;
}

ChatInputAttachmentItem::ChatInputAttachmentItem(std::string path, const Glib::RefPtr<Gdk::Pixbuf> &pb)
    : m_path(std::move(path))
    , m_img(Gtk::make_managed<Gtk::Image>())
    , m_type(ChatSubmitParams::PastedImage) {
    get_style_context()->add_class("attachment-item");

    int outw, outh;
    GetImageDimensions(pb->get_width(), pb->get_height(), outw, outh, AttachmentItemSize, AttachmentItemSize);
    m_img->property_pixbuf() = pb->scale_simple(outw, outh, Gdk::INTERP_BILINEAR);

    set_size_request(AttachmentItemSize, AttachmentItemSize);
    m_box.add(*m_img);
    add(m_box);
    show_all_children();

    SetupMenu();
}

std::string ChatInputAttachmentItem::GetPath() const {
    return m_path;
}

ChatSubmitParams::AttachmentType ChatInputAttachmentItem::GetType() const {
    return m_type;
}

void ChatInputAttachmentItem::SetupMenu() {
    m_menu_remove.set_label("Remove");
    m_menu_remove.signal_activate().connect([this] {
        m_signal_remove.emit();
    });

    m_menu.add(m_menu_remove);
    m_menu.show_all();

    signal_button_press_event().connect([this](GdkEventButton *ev) -> bool {
        if (ev->button == GDK_BUTTON_SECONDARY) {
            m_menu.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
            return true;
        }

        return false;
    });
}

ChatInputAttachmentItem::type_signal_remove ChatInputAttachmentItem::signal_remove() {
    return m_signal_remove;
}

ChatInput::ChatInput()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL) {
    m_input.signal_escape().connect([this] {
        m_attachments.Clear();
        m_attachments_revealer.set_reveal_child(false);
        m_signal_escape.emit();
    });
    m_input.signal_submit().connect([this](const Glib::ustring &input) -> bool {
        ChatSubmitParams data;
        data.Message = input;
        data.Attachments = m_attachments.GetAttachments();

        bool b = m_signal_submit.emit(data);
        if (b) {
            m_attachments_revealer.set_reveal_child(false);
            m_attachments.ClearNoPurge();
        }
        return b;
    });

    m_attachments.set_vexpand(false);

    m_attachments_revealer.set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_SLIDE_UP);
    m_attachments_revealer.add(m_attachments);
    add(m_attachments_revealer);
    add(m_input);
    show_all_children();

    m_input.signal_image_paste().connect([this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
        const bool can_attach_files = m_signal_check_permission.emit(Permission::ATTACH_FILES);

        if (can_attach_files && m_attachments.AddImage(pb))
            m_attachments_revealer.set_reveal_child(true);
    });

    // double hack !
    auto cb = [this](GdkEventKey *e) -> bool {
        return event(reinterpret_cast<GdkEvent *>(e));
    };
    m_input.signal_key_press_event().connect(cb, false);

    m_attachments.signal_emptied().connect([this] {
        m_attachments_revealer.set_reveal_child(false);
    });
}

void ChatInput::InsertText(const Glib::ustring &text) {
    m_input.InsertText(text);
}

Glib::RefPtr<Gtk::TextBuffer> ChatInput::GetBuffer() {
    return m_input.GetBuffer();
}

bool ChatInput::ProcessKeyPress(GdkEventKey *event) {
    return m_input.ProcessKeyPress(event);
}

ChatInput::type_signal_submit ChatInput::signal_submit() {
    return m_signal_submit;
}

ChatInput::type_signal_escape ChatInput::signal_escape() {
    return m_signal_escape;
}

ChatInput::type_signal_check_permission ChatInput::signal_check_permission() {
    return m_signal_check_permission;
}
