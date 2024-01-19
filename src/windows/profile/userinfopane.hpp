#pragma once

#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/overlay.h>
#include <gtkmm/textview.h>

#include "discord/objects.hpp"

class ConnectionItem : public Gtk::EventBox {
public:
    ConnectionItem(const ConnectionData &connection);

private:
    Gtk::Overlay m_overlay;
    Gtk::Box m_box;
    Gtk::Label m_name;
    Gtk::Image *m_image = nullptr;
    Gtk::Image *m_check = nullptr;
};

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

class BioContainer : public Gtk::Box {
public:
    BioContainer();
    void SetBio(const std::string &bio);

private:
    Gtk::Label m_label;
    Gtk::Label m_bio;
};

class ProfileUserInfoPane : public Gtk::Box {
public:
    ProfileUserInfoPane(Snowflake ID);
    void SetProfile(const UserProfileData &data);

    Snowflake UserID;

private:
    Gtk::Label m_created;

    BioContainer m_bio;
    NotesContainer m_note;
    ConnectionsContainer m_conns;
};
