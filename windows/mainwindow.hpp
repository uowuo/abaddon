#pragma once
#include "../components/channels.hpp"
#include <gtkmm.h>

class Abaddon;
class MainWindow : public Gtk::Window {
public:
    MainWindow();
    void SetAbaddon(Abaddon *ptr);

protected:
    Gtk::Box m_main_box;

    ChannelList m_channel_list;

    Gtk::MenuBar m_menu_bar;
    Gtk::MenuItem m_menu_discord;
    Gtk::Menu m_menu_discord_sub;
    Gtk::MenuItem m_menu_discord_connect;

    Abaddon *m_abaddon = nullptr;
};
