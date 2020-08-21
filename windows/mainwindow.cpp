#include "mainwindow.hpp"
#include "../abaddon.hpp"

MainWindow::MainWindow()
    : m_main_box(Gtk::ORIENTATION_VERTICAL)
    , m_content_box(Gtk::ORIENTATION_HORIZONTAL)
    , m_chan_chat_paned(Gtk::ORIENTATION_HORIZONTAL) {
    set_default_size(800, 600);

    m_menu_discord.set_label("Discord");
    m_menu_discord.set_submenu(m_menu_discord_sub);
    m_menu_discord_connect.set_label("Connect");
    m_menu_discord_connect.set_sensitive(false);
    m_menu_discord_disconnect.set_label("Disconnect");
    m_menu_discord_disconnect.set_sensitive(false);
    m_menu_discord_set_token.set_label("Set Token");
    m_menu_discord_sub.append(m_menu_discord_connect);
    m_menu_discord_sub.append(m_menu_discord_disconnect);
    m_menu_discord_sub.append(m_menu_discord_set_token);
    m_menu_discord.set_submenu(m_menu_discord_sub);
    m_menu_bar.append(m_menu_discord);

    m_menu_discord_connect.signal_activate().connect([&] {
        m_abaddon->ActionConnect();
    });

    m_menu_discord_disconnect.signal_activate().connect([&] {
        m_abaddon->ActionDisconnect();
    });

    m_menu_discord_set_token.signal_activate().connect([&] {
        m_abaddon->ActionSetToken();
    });

    m_content_box.set_hexpand(true);
    m_content_box.set_vexpand(true);

    m_main_box.add(m_menu_bar);
    m_main_box.add(m_content_box);

    auto *channel_list = m_channel_list.GetRoot();
    channel_list->set_vexpand(true);
    channel_list->set_size_request(-1, -1);
    m_chan_chat_paned.pack1(*channel_list);
    auto *chat = m_chat.GetRoot();
    chat->set_vexpand(true);
    chat->set_hexpand(true);
    m_chan_chat_paned.pack2(*chat);
    m_chan_chat_paned.set_position(200);
    m_chan_chat_paned.child_property_shrink(*channel_list) = true;
    m_chan_chat_paned.child_property_resize(*channel_list) = true;
    m_content_box.add(m_chan_chat_paned);

    add(m_main_box);

    show_all_children();
}

void MainWindow::UpdateComponents() {
    bool discord_active = m_abaddon->IsDiscordActive();

    // menu
    // Connect
    std::string token = m_abaddon->GetDiscordToken();
    m_menu_discord_connect.set_sensitive(token.size() > 0 && !discord_active);

    // Disconnect
    m_menu_discord_disconnect.set_sensitive(discord_active);

    // channel listing
    if (!discord_active)
        m_channel_list.ClearListing();
    else
        UpdateChannelListing();
}

void MainWindow::UpdateChannelListing() {
    auto &discord = m_abaddon->GetDiscordClient();
    m_channel_list.SetListingFromGuilds(discord.GetGuilds());
}

void MainWindow::UpdateChatWindowContents() {
    auto &discord = m_abaddon->GetDiscordClient();
    m_chat.SetMessages(discord.GetMessagesForChannel(m_chat.GetActiveChannel()));
}

void MainWindow::UpdateChatActiveChannel(Snowflake id) {
    m_chat.SetActiveChannel(id);
}

Snowflake MainWindow::GetChatActiveChannel() const {
    return m_chat.GetActiveChannel();
}

void MainWindow::UpdateChatNewMessage(Snowflake id) {
    if (m_abaddon->GetDiscordClient().GetMessage(id)->ChannelID == GetChatActiveChannel())
        m_chat.AddNewMessage(id);
}

void MainWindow::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
    m_channel_list.SetAbaddon(ptr);
    m_chat.SetAbaddon(ptr);
}
