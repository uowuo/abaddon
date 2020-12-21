#pragma once
#include <gtkmm.h>
#include <functional>
#include "../discord/snowflake.hpp"

class CompleterEntry : public Gtk::ListBoxRow {
public:
    CompleterEntry(const Glib::ustring &completion, int index);
    void SetTextColor(int color); // SetText will reset
    void SetText(const Glib::ustring &text);
    void SetImage(const Glib::RefPtr<Gdk::Pixbuf> &pb);

    int GetIndex() const;
    Glib::ustring GetCompletion() const;

private:
    Glib::ustring m_completion;
    int m_index;
    Gtk::Box m_box;
    Gtk::Label *m_text = nullptr;
    Gtk::Image *m_img = nullptr;
};

class Completer : public Gtk::Revealer {
public:
    Completer();
    Completer(const Glib::RefPtr<Gtk::TextBuffer> &buf);

    void SetBuffer(const Glib::RefPtr<Gtk::TextBuffer> &buf);
    bool ProcessKeyPress(GdkEventKey *e);

    using get_recent_authors_cb = std::function<std::vector<Snowflake>()>;
    void SetGetRecentAuthors(get_recent_authors_cb cb); // maybe a better way idk
    using get_channel_id_cb = std::function<Snowflake()>;
    void SetGetChannelID(get_channel_id_cb cb);

    bool IsShown() const;

private:
    CompleterEntry *CreateEntry(const Glib::ustring &completion);
    void CompleteMentions(const Glib::ustring &term);
    void CompleteEmojis(const Glib::ustring &term);
    void CompleteChannels(const Glib::ustring &term);
    void DoCompletion(Gtk::ListBoxRow *row);

    std::vector<CompleterEntry *> m_entries;

    void OnRowActivate(Gtk::ListBoxRow *row);
    void OnTextBufferChanged();
    Glib::ustring GetTerm();

    Gtk::TextBuffer::iterator m_start;
    Gtk::TextBuffer::iterator m_end;

    Gtk::ScrolledWindow m_scroll;
    Gtk::ListBox m_list;
    Glib::RefPtr<Gtk::TextBuffer> m_buf;

    get_recent_authors_cb m_recent_authors_cb;
    get_channel_id_cb m_channel_id_cb;
};
