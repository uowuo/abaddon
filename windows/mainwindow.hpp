#pragma once
#include "../components/channels.hpp"
#include "../components/chatwindow.hpp"
#include <gtkmm.h>

class Abaddon;
class MainWindow : public Gtk::Window {
public:
    MainWindow();
    void SetAbaddon(Abaddon *ptr);

    void UpdateComponents();
    void UpdateChannelListing();
    void UpdateChatWindowContents();
    void UpdateChatActiveChannel(Snowflake id);

protected:
    Gtk::Box m_main_box;
    Gtk::Box m_content_box;
    Gtk::Paned m_chan_chat_paned;

    ChannelList m_channel_list;
    ChatWindow m_chat;

    Gtk::MenuBar m_menu_bar;
    Gtk::MenuItem m_menu_discord;
    Gtk::Menu m_menu_discord_sub;
    Gtk::MenuItem m_menu_discord_connect;
    Gtk::MenuItem m_menu_discord_disconnect;
    Gtk::MenuItem m_menu_discord_set_token;

    Abaddon *m_abaddon = nullptr;
};
