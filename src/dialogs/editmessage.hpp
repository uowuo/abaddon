#pragma once
#include <string>

class EditMessageDialog : public Gtk::Dialog {
public:
    EditMessageDialog(Gtk::Window &parent);
    Glib::ustring GetContent();
    void SetContent(const Glib::ustring &str);

protected:
    Gtk::Box m_layout;
    Gtk::Button m_ok;
    Gtk::Button m_cancel;
    Gtk::ButtonBox m_bbox;
    Gtk::ScrolledWindow m_scroll;
    Gtk::TextView m_text;

private:
    Glib::ustring m_content;
};
