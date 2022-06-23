#pragma once
#include <gtkmm.h>
#include "discord/chatsubmitparams.hpp"
#include "discord/permissions.hpp"

class ChatInputAttachmentItem : public Gtk::EventBox {
public:
    ChatInputAttachmentItem(const Glib::RefPtr<Gio::File> &file);
    ChatInputAttachmentItem(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gdk::Pixbuf> &pb);

    [[nodiscard]] Glib::RefPtr<Gio::File> GetFile() const;
    [[nodiscard]] ChatSubmitParams::AttachmentType GetType() const;
    [[nodiscard]] std::string GetFilename() const;
    [[nodiscard]] bool IsTemp() const noexcept;
    void RemoveIfTemp();

private:
    void SetupMenu();
    void UpdateTooltip();

    Gtk::Menu m_menu;
    Gtk::MenuItem m_menu_remove;
    Gtk::MenuItem m_menu_set_filename;

    Gtk::Box m_box;
    Gtk::Image *m_img = nullptr;

    Glib::RefPtr<Gio::File> m_file;
    ChatSubmitParams::AttachmentType m_type;
    std::string m_filename;

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
    bool AddFile(const Glib::RefPtr<Gio::File> &file);
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

    type_signal_submit signal_submit();
    type_signal_escape signal_escape();
    type_signal_image_paste signal_image_paste();

private:
    type_signal_submit m_signal_submit;
    type_signal_escape m_signal_escape;
    type_signal_image_paste m_signal_image_paste;
};

class ChatInput : public Gtk::Box {
public:
    ChatInput();

    void InsertText(const Glib::ustring &text);
    Glib::RefPtr<Gtk::TextBuffer> GetBuffer();
    bool ProcessKeyPress(GdkEventKey *event);
    void AddAttachment(const Glib::RefPtr<Gio::File> &file);

private:
    Gtk::Revealer m_attachments_revealer;
    ChatInputAttachmentContainer m_attachments;
    ChatInputText m_input;

public:
    using type_signal_submit = sigc::signal<bool, ChatSubmitParams>;
    using type_signal_escape = sigc::signal<void>;
    using type_signal_check_permission = sigc::signal<bool, Permission>;

    type_signal_submit signal_submit();
    type_signal_escape signal_escape();
    type_signal_check_permission signal_check_permission();

private:
    type_signal_submit m_signal_submit;
    type_signal_escape m_signal_escape;
    type_signal_check_permission m_signal_check_permission;
};
