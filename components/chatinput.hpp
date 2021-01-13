#pragma once
#include <gtkmm.h>

class ChatInput : public Gtk::ScrolledWindow {
public:
    ChatInput();

    void InsertText(const Glib::ustring &text);
    Glib::RefPtr<Gtk::TextBuffer> GetBuffer();
    bool ProcessKeyPress(GdkEventKey *event);

private:

    Gtk::TextView m_textview;

public:
    typedef sigc::signal<void, Glib::ustring> type_signal_submit;

    type_signal_submit signal_submit();

private:
    type_signal_submit m_signal_submit;
};
