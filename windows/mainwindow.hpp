#pragma once
#include "../components/channels.hpp"
#include <gtkmm.h>

class Abaddon;
class MainWindow : public Gtk::Window {
public:
    MainWindow();
    void SetAbaddon(Abaddon *ptr);

    void UpdateMenuStatus();
    void UpdateChannelListing();

protected:
    Gtk::Box m_main_box;
    Gtk::Box m_content_box;

    ChannelList m_channel_list;

    Gtk::MenuBar m_menu_bar;
    Gtk::MenuItem m_menu_discord;
    Gtk::Menu m_menu_discord_sub;
    Gtk::MenuItem m_menu_discord_connect;
    Gtk::MenuItem m_menu_discord_disconnect;
    Gtk::MenuItem m_menu_discord_set_token;

    Abaddon *m_abaddon = nullptr;
};
