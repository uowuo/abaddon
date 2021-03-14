#pragma once
#include <gtkmm.h>

class ChatInput : public Gtk::ScrolledWindow {
public:
    ChatInput();

    void InsertText(const Glib::ustring &text);
    Glib::RefPtr<Gtk::TextBuffer> GetBuffer();
    bool ProcessKeyPress(GdkEventKey *event);

protected:
    void on_grab_focus() override;

private:
    Gtk::TextView m_textview;

public:
    typedef sigc::signal<void, Glib::ustring> type_signal_submit;
    typedef sigc::signal<void> type_signal_escape;

    type_signal_submit signal_submit();
    type_signal_escape signal_escape();

private:
    type_signal_submit m_signal_submit;
    type_signal_escape m_signal_escape;
};
