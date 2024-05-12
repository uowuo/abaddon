#pragma once
#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include "discord/guild.hpp"

struct Message;
struct MessageAckData;
class GuildListGuildItem : public Gtk::EventBox {
public:
    GuildListGuildItem(const GuildData &guild);

    Snowflake ID;

private:
    void UpdateIcon();
    void OnIconFetched(const Glib::RefPtr<Gdk::Pixbuf> &pb);
    void OnMessageCreate(const Message &msg);
    void OnMessageAck(const MessageAckData &data);
    void CheckUnreadStatus();

    Gtk::Box m_box;
    Gtk::Image m_image;
};
