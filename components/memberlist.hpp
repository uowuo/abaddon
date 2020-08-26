#pragma once
#include <gtkmm.h>

class MemberList {
public:
    MemberList();
    Gtk::Widget *GetRoot() const;

private:
    Gtk::Box *m_main;
};
