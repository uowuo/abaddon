#pragma once

#include <gtkmm/dialog.h>
#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <gtkmm/box.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/button.h>
#include "discord/snowflake.hpp"
#include "discord/objects.hpp"
#include "discord/channel.hpp"

class CallDialog : public Gtk::Dialog {
public:
    CallDialog(Gtk::Window &parent, Snowflake channel_id, bool is_group_call);

    bool GetAccepted() const;

protected:
    void OnAccept();
    void OnReject();

    bool m_accepted = false;
    Snowflake m_channel_id;
    bool m_is_group_call;

    Gtk::Box m_main_layout;
    Gtk::Box m_info_layout;
    Gtk::Image m_avatar;
    Gtk::Label m_title_label;
    Gtk::Label m_info_label;
    Gtk::ButtonBox m_button_box;
    Gtk::Button m_accept_button;
    Gtk::Button m_reject_button;
};
