#pragma once
#include <gtkmm.h>

class ChannelList {
public:
    ChannelList();
    Gtk::Widget *GetRoot() const;

protected:
    Gtk::ListBox *m_list;
    Gtk::ScrolledWindow *m_main;
};
