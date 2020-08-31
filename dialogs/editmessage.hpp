#pragma once
#include <gtkmm.h>
#include <string>

class EditMessageDialog : public Gtk::Dialog {
public:
    EditMessageDialog(Gtk::Window &parent);
    std::string GetContent();

protected:
    Gtk::Box m_layout;
    Gtk::Button m_ok;
    Gtk::Button m_cancel;
    Gtk::ButtonBox m_bbox;
    Gtk::ScrolledWindow m_scroll;
    Gtk::TextView m_text;

private:
    std::string m_content;
};
