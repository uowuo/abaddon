#pragma once
#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/image.h>
#include <gtkmm/revealer.h>
#include <gtkmm/stack.h>

#include "guildlistguilditem.hpp"
#include "discord/usersettings.hpp"

class GuildListGuildItem;

class GuildListFolderButton : public Gtk::Grid {
public:
    GuildListFolderButton();
    void SetGuilds(const std::vector<Snowflake> &guild_ids);

private:
    Gtk::Image m_images[2][2];
};

class GuildListFolderItem : public Gtk::VBox {
public:
    GuildListFolderItem(const UserSettingsGuildFoldersEntry &folder);

    void AddGuildWidget(GuildListGuildItem *widget);

private:
    void OnMessageCreate(const Message &msg);
    void OnMessageAck(const MessageAckData &data);
    void CheckUnreadStatus();

    std::vector<Snowflake> m_guild_ids;

    Gtk::Stack m_stack;
    GuildListFolderButton m_grid;
    Gtk::Image m_icon;

    Gtk::EventBox m_ev;
    Gtk::Image m_image;
    Gtk::Revealer m_revealer;
    Gtk::VBox m_box;
};
