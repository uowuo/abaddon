#include "mainwindow.hpp"
#include "../abaddon.hpp"

MainWindow::MainWindow()
    : m_main_box(Gtk::ORIENTATION_VERTICAL) {
    set_default_size(800, 600);

    m_menu_discord.set_label("Discord");
    m_menu_discord.set_submenu(m_menu_discord_sub);
    m_menu_discord_connect.set_label("Connect");
    m_menu_discord_sub.append(m_menu_discord_connect);
    m_menu_discord.set_submenu(m_menu_discord_sub);
    m_menu_bar.append(m_menu_discord);

    m_menu_discord_connect.signal_activate().connect([&] {
        m_abaddon->ActionConnect(); // this feels maybe not too smart
    });

    m_main_box.add(m_menu_bar);

    auto *channel_list = m_channel_list.GetRoot();
    m_main_box.add(*channel_list);

    add(m_main_box);

    show_all_children();
}

void MainWindow::SetAbaddon(Abaddon* ptr) {
    m_abaddon = ptr;
}
