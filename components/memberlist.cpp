#include "memberlist.hpp"

MemberList::MemberList() {
    m_main = Gtk::manage(new Gtk::Box);
}

Gtk::Widget *MemberList::GetRoot() const {
    return m_main;
}
