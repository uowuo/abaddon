#include "chatinput.hpp"
#include <filesystem>
#include <gtkmm/dialog.h>
#include <gtkmm/filechoosernative.h>
#include "abaddon.hpp"
#include "constants.hpp"
#include "misc/events.hpp"
#include "util.hpp"

ChatInputText::ChatInputText() {
    get_style_context()->add_class("message-input");
    set_propagate_natural_height(true);
    set_min_content_height(20);
    set_max_content_height(250);
    set_policy(Gtk::POLICY_EXTERNAL, Gtk::POLICY_AUTOMATIC);

    // hack
    auto cb = [this](GdkEventKey *e) -> bool {
        // we cant use Widget::event here specifically or else for some reason
        // it prevents the default binding set from activating sometimes
        // specifically event() will return true (why) preventing default handler from running
        return m_signal_key_press_proxy.emit(e);
    };
    m_textview.signal_key_press_event().connect(cb, false);
    m_textview.set_hexpand(false);
    m_textview.set_halign(Gtk::ALIGN_FILL);
    m_textview.set_valign(Gtk::ALIGN_FILL);
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

    const auto shortcut = EventsUtil::shortcut_key(event);

#ifdef __APPLE__
    if ((event->state & GDK_MOD2_MASK) && shortcut == GDK_KEY_v) {
        return CheckHandleClipboardPaste();
    }
#else
    if ((event->state & GDK_CONTROL_MASK) && shortcut == GDK_KEY_v) {
        return CheckHandleClipboardPaste();
    }
#endif

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

ChatInputText::type_signal_key_press_proxy ChatInputText::signal_key_press_proxy() {
    return m_signal_key_press_proxy;
}

ChatInputTextContainer::ChatInputTextContainer() {
    // triple hack !!!
    auto cb = [this](GdkEventKey *e) -> bool {
        return event(reinterpret_cast<GdkEvent *>(e));
    };
    m_input.signal_key_press_proxy().connect(cb);

    m_upload_button.set_image(m_upload_img);
    m_upload_button.set_halign(Gtk::ALIGN_CENTER);
    m_upload_button.set_valign(Gtk::ALIGN_CENTER);
    m_upload_button.get_style_context()->add_class(GTK_STYLE_CLASS_FLAT);

    m_upload_img.property_icon_name() = "document-send-symbolic";
    m_upload_img.property_icon_size() = Gtk::ICON_SIZE_LARGE_TOOLBAR;
    m_upload_img.get_style_context()->add_class("message-input-browse-icon");

    m_upload_button.signal_clicked().connect([this]() {
        ShowFileChooser();
        m_input.grab_focus();
    });

    m_upload_box.pack_start(m_upload_button);
    pack_start(m_upload_box, false, false);
    pack_start(m_input);

    show_all_children();
}

void ChatInputTextContainer::ShowFileChooser() {
    auto dlg = Gtk::FileChooserNative::create("Choose file", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg->set_select_multiple(true);
    dlg->set_modal(true);

    dlg->signal_response().connect([this, dlg](int response) {
        if (response == Gtk::RESPONSE_ACCEPT) {
            for (const auto &file : dlg->get_files()) {
                m_signal_add_attachment.emit(file);
            }
        }
    });

    dlg->run();
}

ChatInputText &ChatInputTextContainer::Get() {
    return m_input;
}

void ChatInputTextContainer::ShowChooserIcon() {
    m_upload_button.show();
}

void ChatInputTextContainer::HideChooserIcon() {
    m_upload_button.hide();
}

ChatInputTextContainer::type_signal_add_attachment ChatInputTextContainer::signal_add_attachment() {
    return m_signal_add_attachment;
}

ChatInputAttachmentContainer::ChatInputAttachmentContainer()
    : m_box(Gtk::ORIENTATION_HORIZONTAL) {
    get_style_context()->add_class("attachment-container");

    m_box.set_halign(Gtk::ALIGN_START);

    add(m_box);
    m_box.show();

    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);
    set_vexpand(true);
    set_size_request(-1, AttachmentItemSize + 10);
}

void ChatInputAttachmentContainer::Clear() {
    for (auto *item : m_attachments) {
        item->RemoveIfTemp();
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

    auto *item = Gtk::make_managed<ChatInputAttachmentItem>(Gio::File::create_for_path(path), pb);
    item->set_valign(Gtk::ALIGN_FILL);
    item->set_vexpand(true);
    item->set_margin_bottom(5);
    item->show();
    m_box.add(*item);

    m_attachments.push_back(item);

    item->signal_item_removed().connect([this, item] {
        item->RemoveIfTemp();
        if (auto it = std::find(m_attachments.begin(), m_attachments.end(), item); it != m_attachments.end())
            m_attachments.erase(it);
        delete item;
        if (m_attachments.empty())
            m_signal_emptied.emit();
    });

    return true;
}

bool ChatInputAttachmentContainer::AddFile(const Glib::RefPtr<Gio::File> &file, Glib::RefPtr<Gdk::Pixbuf> pb) {
    if (m_attachments.size() == 10) return false;

    ChatInputAttachmentItem *item;
    if (pb)
        item = Gtk::make_managed<ChatInputAttachmentItem>(file, pb, true);
    else
        item = Gtk::make_managed<ChatInputAttachmentItem>(file);
    item->set_valign(Gtk::ALIGN_FILL);
    item->set_vexpand(true);
    item->set_margin_bottom(5);
    item->show();
    m_box.add(*item);

    m_attachments.push_back(item);

    item->signal_item_removed().connect([this, item] {
        if (auto it = std::find(m_attachments.begin(), m_attachments.end(), item); it != m_attachments.end())
            m_attachments.erase(it);
        delete item;
        if (m_attachments.empty())
            m_signal_emptied.emit();
    });

    return true;
}

std::vector<ChatSubmitParams::Attachment> ChatInputAttachmentContainer::GetAttachments() const {
    std::vector<ChatSubmitParams::Attachment> ret;
    for (auto *x : m_attachments) {
        if (!x->GetFile()->query_exists())
            puts("bad!");
        ret.push_back({ x->GetFile(), x->GetType(), x->GetFilename(), x->GetDescription() });
    }
    return ret;
}

ChatInputAttachmentContainer::type_signal_emptied ChatInputAttachmentContainer::signal_emptied() {
    return m_signal_emptied;
}

ChatInputAttachmentItem::ChatInputAttachmentItem(const Glib::RefPtr<Gio::File> &file)
    : m_file(file)
    , m_img(Gtk::make_managed<Gtk::Image>())
    , m_type(ChatSubmitParams::ExtantFile)
    , m_box(Gtk::ORIENTATION_VERTICAL) {
    get_style_context()->add_class("attachment-item");

    set_size_request(AttachmentItemSize, AttachmentItemSize);
    set_halign(Gtk::ALIGN_START);
    m_box.set_hexpand(true);
    m_box.set_vexpand(true);
    m_box.set_halign(Gtk::ALIGN_FILL);
    m_box.set_valign(Gtk::ALIGN_FILL);
    m_box.add(*m_img);
    m_box.add(m_label);
    add(m_box);
    show_all_children();

    m_label.set_valign(Gtk::ALIGN_END);
    m_label.set_max_width_chars(0); // will constrain to given size
    m_label.set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
    m_label.set_margin_start(7);
    m_label.set_margin_end(7);

    m_img->set_vexpand(true);
    m_img->property_icon_name() = "document-send-symbolic";
    m_img->property_icon_size() = Gtk::ICON_SIZE_DIALOG; // todo figure out how to not use this weird property??? i dont know how icons work (screw your theme)

    SetFilenameFromFile();

    SetupMenu();
    UpdateTooltip();
}

ChatInputAttachmentItem::ChatInputAttachmentItem(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gdk::Pixbuf> &pb, bool is_extant)
    : m_file(file)
    , m_img(Gtk::make_managed<Gtk::Image>())
    , m_type(is_extant ? ChatSubmitParams::ExtantFile : ChatSubmitParams::PastedImage)
    , m_filename("unknown.png")
    , m_is_image(true)
    , m_label("unknown.png")
    , m_box(Gtk::ORIENTATION_VERTICAL) {
    get_style_context()->add_class("attachment-item");

    int outw, outh;
    GetImageDimensions(pb->get_width(), pb->get_height(), outw, outh, AttachmentItemSize, AttachmentItemSize);
    m_img->property_pixbuf() = pb->scale_simple(outw, outh, Gdk::INTERP_BILINEAR);

    set_size_request(AttachmentItemSize, AttachmentItemSize);
    set_halign(Gtk::ALIGN_START);
    m_box.set_hexpand(true);
    m_box.set_vexpand(true);
    m_box.set_halign(Gtk::ALIGN_FILL);
    m_box.set_valign(Gtk::ALIGN_FILL);
    m_box.add(*m_img);
    m_box.add(m_label);
    add(m_box);
    show_all_children();

    m_label.set_valign(Gtk::ALIGN_END);
    m_label.set_max_width_chars(0); // will constrain to given size
    m_label.set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
    m_label.set_margin_start(7);
    m_label.set_margin_end(7);

    m_img->set_vexpand(true);

    if (is_extant)
        SetFilenameFromFile();

    SetupMenu();
    UpdateTooltip();
}

Glib::RefPtr<Gio::File> ChatInputAttachmentItem::GetFile() const {
    return m_file;
}

ChatSubmitParams::AttachmentType ChatInputAttachmentItem::GetType() const {
    return m_type;
}

std::string ChatInputAttachmentItem::GetFilename() const {
    return m_filename;
}

std::optional<std::string> ChatInputAttachmentItem::GetDescription() const {
    return m_description.empty() ? std::nullopt : std::optional<std::string>(m_description);
}

bool ChatInputAttachmentItem::IsTemp() const noexcept {
    return m_type == ChatSubmitParams::PastedImage;
}

bool ChatInputAttachmentItem::IsImage() const noexcept {
    return m_is_image;
}

void ChatInputAttachmentItem::RemoveIfTemp() {
    if (IsTemp())
        m_file->remove();
}

void ChatInputAttachmentItem::SetFilenameFromFile() {
    auto info = m_file->query_info(G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
    m_filename = info->get_attribute_string(G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
    m_label.set_text(m_filename);
}

void ChatInputAttachmentItem::SetupMenu() {
    m_menu_remove.set_label("Remove");
    m_menu_remove.signal_activate().connect([this] {
        m_signal_item_removed.emit();
    });

    m_menu_set_filename.set_label("Change Filename");
    m_menu_set_filename.signal_activate().connect([this] {
        const auto name = Abaddon::Get().ShowTextPrompt("Enter new filename for attachment", "Enter filename", m_filename);
        if (name.has_value()) {
            m_filename = *name;
            m_label.set_text(m_filename);
            UpdateTooltip();
        }
    });

    m_menu_set_alt_text.set_label("Change Alt-Text");
    m_menu_set_alt_text.signal_activate().connect([this]() {
        const auto description = Abaddon::Get().ShowTextPrompt("Enter description (alt-text) for attachment", "Enter alt-text", m_description);
        if (description.has_value()) {
            m_description = *description;
        }
    });

    m_menu.add(m_menu_set_filename);
    m_menu.add(m_menu_set_alt_text);
    m_menu.add(m_menu_remove);
    m_menu.show_all();

    signal_button_press_event().connect([this](GdkEventButton *ev) -> bool {
        if (ev->button == GDK_BUTTON_SECONDARY) {
            m_menu_set_alt_text.set_visible(IsImage());
            m_menu.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
            return true;
        }

        return false;
    });
}

void ChatInputAttachmentItem::UpdateTooltip() {
    set_tooltip_text(m_filename);
}

ChatInputAttachmentItem::type_signal_item_removed ChatInputAttachmentItem::signal_item_removed() {
    return m_signal_item_removed;
}

ChatInput::ChatInput()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL) {
    m_input.signal_add_attachment().connect(sigc::mem_fun(*this, &ChatInput::AddAttachment));

    m_input.Get().signal_escape().connect([this] {
        m_attachments.Clear();
        m_attachments_revealer.set_reveal_child(false);
        m_signal_escape.emit();
    });

    m_input.Get().signal_submit().connect([this](const Glib::ustring &input) -> bool {
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

    m_input.Get().signal_image_paste().connect([this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
        if (CanAttachFiles() && m_attachments.AddImage(pb))
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

    SetActiveChannel(Snowflake::Invalid);
}

void ChatInput::InsertText(const Glib::ustring &text) {
    m_input.Get().InsertText(text);
}

void ChatInput::Clear() {
    GetBuffer()->set_text("");
}

Glib::RefPtr<Gtk::TextBuffer> ChatInput::GetBuffer() {
    return m_input.Get().GetBuffer();
}

bool ChatInput::ProcessKeyPress(GdkEventKey *event) {
    return m_input.Get().ProcessKeyPress(event);
}

void ChatInput::AddAttachment(const Glib::RefPtr<Gio::File> &file) {
    if (!CanAttachFiles()) return;

    std::string content_type;

    try {
        const auto info = file->query_info(G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
        content_type = info->get_attribute_string(G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
    } catch (const Gio::Error &err) {
        printf("io error: %s\n", err.what().c_str());
        return;
    } catch (...) {
        puts("attachment query exception");
        return;
    }

    static const std::unordered_set<std::string> image_exts {
        ".png",
        ".jpg",
    };

    if (image_exts.find(content_type) != image_exts.end()) {
        if (AddFileAsImageAttachment(file)) {
            m_attachments_revealer.set_reveal_child(true);
            m_input.Get().grab_focus();
        }
    } else if (m_attachments.AddFile(file)) {
        m_attachments_revealer.set_reveal_child(true);
        m_input.Get().grab_focus();
    }
}

void ChatInput::IndicateTooLarge() {
    m_input.Get().get_style_context()->add_class("bad-input");
    const auto cb = [this] {
        m_input.Get().get_style_context()->remove_class("bad-input");
    };
    Glib::signal_timeout().connect_seconds_once(sigc::track_obj(cb, *this), 2);
}

void ChatInput::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
    if (CanAttachFiles()) {
        m_input.Get().get_style_context()->add_class("with-browse-icon");
        m_input.ShowChooserIcon();
    } else {
        m_input.Get().get_style_context()->remove_class("with-browse-icon");
        m_input.HideChooserIcon();
    }
}

void ChatInput::StartReplying() {
    m_input.Get().grab_focus();
    m_input.Get().get_style_context()->add_class("replying");
}

void ChatInput::StopReplying() {
    m_input.Get().get_style_context()->remove_class("replying");
}

void ChatInput::StartEditing(const Message &message) {
    m_is_editing = true;
    m_input.Get().grab_focus();
    m_input.Get().get_style_context()->add_class("editing");
    GetBuffer()->set_text(message.Content);
    m_attachments.Clear();
    m_attachments_revealer.set_reveal_child(false);
}

void ChatInput::StopEditing() {
    m_is_editing = false;
    m_input.Get().get_style_context()->remove_class("editing");
}

bool ChatInput::IsEmpty() {
    return GetBuffer()->get_char_count() == 0;
}

bool ChatInput::AddFileAsImageAttachment(const Glib::RefPtr<Gio::File> &file) {
    try {
        const auto read_stream = file->read();
        if (!read_stream) return false;
        const auto pb = Gdk::Pixbuf::create_from_stream(read_stream);
        return m_attachments.AddFile(file, pb);
    } catch (...) {
        return m_attachments.AddFile(file);
    }
}

bool ChatInput::CanAttachFiles() {
    return !m_is_editing && Abaddon::Get().GetDiscordClient().HasSelfChannelPermission(m_active_channel, Permission::ATTACH_FILES | Permission::SEND_MESSAGES);
}

ChatInput::type_signal_submit ChatInput::signal_submit() {
    return m_signal_submit;
}

ChatInput::type_signal_escape ChatInput::signal_escape() {
    return m_signal_escape;
}
