#pragma once
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/revealer.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include "discord/chatsubmitparams.hpp"
#include "discord/message.hpp"
#include "discord/permissions.hpp"

class ChatInputAttachmentItem : public Gtk::EventBox {
public:
    ChatInputAttachmentItem(const Glib::RefPtr<Gio::File> &file);
    ChatInputAttachmentItem(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gdk::Pixbuf> &pb, bool is_extant = false);

    Glib::RefPtr<Gio::File> GetFile() const;
    ChatSubmitParams::AttachmentType GetType() const;
    std::string GetFilename() const;
    std::optional<std::string> GetDescription() const;
    bool IsTemp() const noexcept;
    bool IsImage() const noexcept;

    void RemoveIfTemp();

private:
    void SetFilenameFromFile();
    void SetupMenu();
    void UpdateTooltip();

    Gtk::Menu m_menu;
    Gtk::MenuItem m_menu_remove;
    Gtk::MenuItem m_menu_set_filename;
    Gtk::MenuItem m_menu_set_alt_text;

    Gtk::Box m_box;
    Gtk::Label m_label;
    Gtk::Image *m_img = nullptr;

    Glib::RefPtr<Gio::File> m_file;
    ChatSubmitParams::AttachmentType m_type;
    std::string m_filename;
    std::string m_description;
    bool m_is_image = false;

private:
    using type_signal_item_removed = sigc::signal<void>;

    type_signal_item_removed m_signal_item_removed;

public:
    type_signal_item_removed signal_item_removed();
};

class ChatInputAttachmentContainer : public Gtk::ScrolledWindow {
public:
    ChatInputAttachmentContainer();

    void Clear();
    void ClearNoPurge();
    bool AddImage(const Glib::RefPtr<Gdk::Pixbuf> &pb);
    bool AddFile(const Glib::RefPtr<Gio::File> &file, Glib::RefPtr<Gdk::Pixbuf> pb = {});
    [[nodiscard]] std::vector<ChatSubmitParams::Attachment> GetAttachments() const;

private:
    std::vector<ChatInputAttachmentItem *> m_attachments;

    Gtk::Box m_box;

private:
    using type_signal_emptied = sigc::signal<void>;

    type_signal_emptied m_signal_emptied;

public:
    type_signal_emptied signal_emptied();
};

class ChatInputText : public Gtk::ScrolledWindow {
public:
    ChatInputText();

    void InsertText(const Glib::ustring &text);
    Glib::RefPtr<Gtk::TextBuffer> GetBuffer();
    bool ProcessKeyPress(GdkEventKey *event);

protected:
    void on_grab_focus() override;

private:
    Gtk::TextView m_textview;

    bool CheckHandleClipboardPaste();

public:
    using type_signal_submit = sigc::signal<bool, Glib::ustring>;
    using type_signal_escape = sigc::signal<void>;
    using type_signal_image_paste = sigc::signal<void, Glib::RefPtr<Gdk::Pixbuf>>;
    using type_signal_key_press_proxy = sigc::signal<bool, GdkEventKey *>;

    type_signal_submit signal_submit();
    type_signal_escape signal_escape();
    type_signal_image_paste signal_image_paste();
    type_signal_key_press_proxy signal_key_press_proxy();

private:
    type_signal_submit m_signal_submit;
    type_signal_escape m_signal_escape;
    type_signal_image_paste m_signal_image_paste;
    type_signal_key_press_proxy m_signal_key_press_proxy;
};

// file upload, text
class ChatInputTextContainer : public Gtk::Box {
public:
    ChatInputTextContainer();

    // not proxying everythign lol!!
    ChatInputText &Get();

    void ShowChooserIcon();
    void HideChooserIcon();

private:
    void ShowFileChooser();

    Gtk::Box m_upload_box;
    Gtk::Button m_upload_button;
    Gtk::Image m_upload_img;
    ChatInputText m_input;

public:
    using type_signal_add_attachment = sigc::signal<void, Glib::RefPtr<Gio::File>>;
    type_signal_add_attachment signal_add_attachment();

private:
    type_signal_add_attachment m_signal_add_attachment;
};

class ChatInput : public Gtk::Box {
public:
    ChatInput();

    void InsertText(const Glib::ustring &text);
    void Clear();
    Glib::RefPtr<Gtk::TextBuffer> GetBuffer();
    bool ProcessKeyPress(GdkEventKey *event);
    void AddAttachment(const Glib::RefPtr<Gio::File> &file);
    void IndicateTooLarge();

    void SetActiveChannel(Snowflake id);

    void StartReplying();
    void StopReplying();

    void StartEditing(const Message &message);
    void StopEditing();

    bool IsEmpty();

private:
    bool AddFileAsImageAttachment(const Glib::RefPtr<Gio::File> &file);
    bool CanAttachFiles();

    Gtk::Revealer m_attachments_revealer;
    ChatInputAttachmentContainer m_attachments;
    ChatInputTextContainer m_input;

    Snowflake m_active_channel;

    bool m_is_editing = false;

public:
    using type_signal_submit = sigc::signal<bool, ChatSubmitParams>;
    using type_signal_escape = sigc::signal<void>;

    type_signal_submit signal_submit();
    type_signal_escape signal_escape();

private:
    type_signal_submit m_signal_submit;
    type_signal_escape m_signal_escape;
};
