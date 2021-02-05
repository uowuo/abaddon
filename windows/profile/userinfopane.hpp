#pragma once
#include <gtkmm.h>
#include "../../discord/objects.hpp"

class ConnectionsContainer : public Gtk::Grid {
public:
    ConnectionsContainer();
    void SetConnections(const std::vector<ConnectionData> &connections);
};

class NotesContainer : public Gtk::Box {
public:
    NotesContainer();
    void SetNote(const std::string &note);

private:
    void UpdateNote();
    bool OnNoteKeyPress(GdkEventKey *event);

    Gtk::Label m_label;
    Gtk::TextView m_note;

    typedef sigc::signal<void, Glib::ustring> type_signal_update_note;
    type_signal_update_note m_signal_update_note;

public:
    type_signal_update_note signal_update_note();
};

class ProfileUserInfoPane : public Gtk::Box {
public:
    ProfileUserInfoPane(Snowflake ID);
    void SetConnections(const std::vector<ConnectionData> &connections);

    Snowflake UserID;

private:
    NotesContainer m_note;
    ConnectionsContainer m_conns;
};
