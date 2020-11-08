#include "mainwindow.hpp"
#include "../abaddon.hpp"

MainWindow::MainWindow()
    : m_main_box(Gtk::ORIENTATION_VERTICAL)
    , m_content_box(Gtk::ORIENTATION_HORIZONTAL)
    , m_chan_chat_paned(Gtk::ORIENTATION_HORIZONTAL)
    , m_chat_members_paned(Gtk::ORIENTATION_HORIZONTAL) {
    set_default_size(1200, 800);

    m_menu_discord.set_label("Discord");
    m_menu_discord.set_submenu(m_menu_discord_sub);
    m_menu_discord_connect.set_label("Connect");
    m_menu_discord_connect.set_sensitive(false);
    m_menu_discord_disconnect.set_label("Disconnect");
    m_menu_discord_disconnect.set_sensitive(false);
    m_menu_discord_set_token.set_label("Set Token");
    m_menu_discord_join_guild.set_label("Join Guild");
    m_menu_discord_join_guild.set_sensitive(false);
    m_menu_discord_set_status.set_label("Set Status");
    m_menu_discord_set_status.set_sensitive(false);
    m_menu_discord_sub.append(m_menu_discord_connect);
    m_menu_discord_sub.append(m_menu_discord_disconnect);
    m_menu_discord_sub.append(m_menu_discord_set_token);
    m_menu_discord_sub.append(m_menu_discord_join_guild);
    m_menu_discord_sub.append(m_menu_discord_set_status);
    m_menu_discord.set_submenu(m_menu_discord_sub);

    m_menu_file.set_label("File");
    m_menu_file.set_submenu(m_menu_file_sub);
    m_menu_file_reload_css.set_label("Reload CSS");
    m_menu_file_sub.append(m_menu_file_reload_css);

    m_menu_bar.append(m_menu_file);
    m_menu_bar.append(m_menu_discord);

    m_menu_discord_connect.signal_activate().connect([this] {
        m_signal_action_connect.emit();
    });

    m_menu_discord_disconnect.signal_activate().connect([this] {
        m_signal_action_disconnect.emit();
    });

    m_menu_discord_set_token.signal_activate().connect([this] {
        m_signal_action_set_token.emit();
    });

    m_menu_discord_join_guild.signal_activate().connect([this] {
        m_signal_action_join_guild.emit();
    });

    m_menu_file_reload_css.signal_activate().connect([this] {
        m_signal_action_reload_css.emit();
    });

    m_menu_discord_set_status.signal_activate().connect([this] {
        m_signal_action_set_status.emit();
    });

    m_content_box.set_hexpand(true);
    m_content_box.set_vexpand(true);

    m_main_box.add(m_menu_bar);
    m_main_box.add(m_content_box);

    auto *channel_list = m_channel_list.GetRoot();
    auto *member_list = m_members.GetRoot();
    auto *chat = m_chat.GetRoot();

    m_members.signal_action_show_user_menu().connect([this](const GdkEvent *event, Snowflake id, Snowflake guild_id) {
        m_signal_action_show_user_menu.emit(event, id, guild_id);
    });

    m_chat.signal_action_open_user_menu().connect([this](const GdkEvent *event, Snowflake id, Snowflake guild_id) {
        m_signal_action_show_user_menu.emit(event, id, guild_id);
    });

    chat->set_vexpand(true);
    chat->set_hexpand(true);

    channel_list->set_vexpand(true);
    channel_list->set_size_request(-1, -1);

    member_list->set_vexpand(true);

    m_chan_chat_paned.pack1(*channel_list);
    m_chan_chat_paned.pack2(m_chat_members_paned);
    m_chan_chat_paned.child_property_shrink(*channel_list) = true;
    m_chan_chat_paned.child_property_resize(*channel_list) = true;
    m_chan_chat_paned.set_position(200);
    m_content_box.add(m_chan_chat_paned);

    m_chat_members_paned.pack1(*chat);
    m_chat_members_paned.pack2(*member_list);
    m_chat_members_paned.child_property_shrink(*member_list) = true;
    m_chat_members_paned.child_property_resize(*member_list) = true;
    int w, h;
    get_default_size(w, h); // :s
    m_chat_members_paned.set_position(w - m_chan_chat_paned.get_position() - 150);

    add(m_main_box);

    show_all_children();
}

void MainWindow::UpdateComponents() {
    bool discord_active = Abaddon::Get().IsDiscordActive();

    std::string token = Abaddon::Get().GetDiscordToken();
    m_menu_discord_connect.set_sensitive(token.size() > 0 && !discord_active);
    m_menu_discord_disconnect.set_sensitive(discord_active);
    m_menu_discord_join_guild.set_sensitive(discord_active);
    m_menu_discord_set_token.set_sensitive(!discord_active);
    m_menu_discord_set_status.set_sensitive(discord_active);

    if (!discord_active) {
        m_channel_list.Clear();
        m_chat.Clear();
        m_members.Clear();
    } else {
        UpdateChannelListing();
        m_members.UpdateMemberList();
    }
}

void MainWindow::UpdateMembers() {
    m_members.UpdateMemberList();
}

void MainWindow::UpdateChannelListing() {
    m_channel_list.UpdateListing();
}

void MainWindow::UpdateChannelsNewGuild(Snowflake id) {
    m_channel_list.UpdateNewGuild(id);
}

void MainWindow::UpdateChannelsRemoveGuild(Snowflake id) {
    m_channel_list.UpdateRemoveGuild(id);
}

void MainWindow::UpdateChannelsRemoveChannel(Snowflake id) {
    m_channel_list.UpdateRemoveChannel(id);
}

void MainWindow::UpdateChannelsUpdateChannel(Snowflake id) {
    m_channel_list.UpdateChannel(id);
}

void MainWindow::UpdateChannelsCreateChannel(Snowflake id) {
    m_channel_list.UpdateCreateChannel(id);
}

void MainWindow::UpdateChannelsUpdateGuild(Snowflake id) {
    m_channel_list.UpdateGuild(id);
}

void MainWindow::UpdateChatWindowContents() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto allmsgs = discord.GetMessagesForChannel(m_chat.GetActiveChannel());
    if (allmsgs.size() > 50) {
        std::vector<Snowflake> msgvec(allmsgs.begin(), allmsgs.end());
        std::vector<Snowflake> cutvec(msgvec.end() - 50, msgvec.end());
        std::set<Snowflake> msgs;
        for (const auto s : cutvec)
            msgs.insert(s);
        m_chat.SetMessages(msgs);
    } else {
        m_chat.SetMessages(allmsgs);
    }
    m_members.UpdateMemberList();
}

void MainWindow::UpdateChatActiveChannel(Snowflake id) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    m_chat.SetActiveChannel(id);
    m_members.SetActiveChannel(id);
    m_channel_list.SetActiveChannel(id);
}

Snowflake MainWindow::GetChatActiveChannel() const {
    return m_chat.GetActiveChannel();
}

void MainWindow::UpdateChatNewMessage(Snowflake id) {
    if (Abaddon::Get().GetDiscordClient().GetMessage(id)->ChannelID == GetChatActiveChannel()) {
        m_chat.AddNewMessage(id);
    }
}

void MainWindow::UpdateChatMessageDeleted(Snowflake id, Snowflake channel_id) {
    if (channel_id == GetChatActiveChannel())
        m_chat.DeleteMessage(id);
}

void MainWindow::UpdateChatMessageUpdated(Snowflake id, Snowflake channel_id) {
    if (channel_id == GetChatActiveChannel())
        m_chat.UpdateMessage(id);
}

void MainWindow::UpdateChatPrependHistory(const std::vector<Snowflake> &msgs) {
    m_chat.AddNewHistory(msgs);
}

void MainWindow::InsertChatInput(std::string text) {
    m_chat.InsertChatInput(text);
}

Snowflake MainWindow::GetChatOldestListedMessage() {
    return m_chat.GetOldestListedMessage();
}

ChannelList *MainWindow::GetChannelList() {
    return &m_channel_list;
}

ChatWindow *MainWindow::GetChatWindow() {
    return &m_chat;
}

MemberList *MainWindow::GetMemberList() {
    return &m_members;
}

MainWindow::type_signal_action_connect MainWindow::signal_action_connect() {
    return m_signal_action_connect;
}

MainWindow::type_signal_action_disconnect MainWindow::signal_action_disconnect() {
    return m_signal_action_disconnect;
}

MainWindow::type_signal_action_set_token MainWindow::signal_action_set_token() {
    return m_signal_action_set_token;
}

MainWindow::type_signal_action_reload_css MainWindow::signal_action_reload_css() {
    return m_signal_action_reload_css;
}

MainWindow::type_signal_action_join_guild MainWindow::signal_action_join_guild() {
    return m_signal_action_join_guild;
}

MainWindow::type_signal_action_set_status MainWindow::signal_action_set_status() {
    return m_signal_action_set_status;
}

MainWindow::type_signal_action_show_user_menu MainWindow::signal_action_show_user_menu() {
    return m_signal_action_show_user_menu;
}
