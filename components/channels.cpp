#include "channels.hpp"

ChannelList::ChannelList() {
    m_main = Gtk::manage(new Gtk::ScrolledWindow);
    m_list = Gtk::manage(new Gtk::ListBox);

    m_main->add(*m_list);
}

Gtk::Widget* ChannelList::GetRoot() const {
    return m_main;
}
