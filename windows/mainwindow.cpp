#include "mainwindow.hpp"
#include "../abaddon.hpp"

MainWindow::MainWindow()
    : m_main_box(Gtk::ORIENTATION_VERTICAL)
    , m_content_box(Gtk::ORIENTATION_HORIZONTAL)
    , m_chan_content_paned(Gtk::ORIENTATION_HORIZONTAL)
    , m_content_members_paned(Gtk::ORIENTATION_HORIZONTAL) {
    set_default_size(1200, 800);
    get_style_context()->add_class("app-window");

    m_menu_discord.set_label("Discord");
    m_menu_discord.set_submenu(m_menu_discord_sub);
    m_menu_discord_connect.set_label("Connect");
    m_menu_discord_connect.set_sensitive(false);
    m_menu_discord_disconnect.set_label("Disconnect");
    m_menu_discord_disconnect.set_sensitive(false);
    m_menu_discord_set_token.set_label("Set Token");
    m_menu_discord_join_guild.set_label("Accept Invite");
    m_menu_discord_join_guild.set_sensitive(false);
    m_menu_discord_set_status.set_label("Set Status");
    m_menu_discord_set_status.set_sensitive(false);
    m_menu_discord_add_recipient.set_label("Add user to DM");
    m_menu_discord_sub.append(m_menu_discord_connect);
    m_menu_discord_sub.append(m_menu_discord_disconnect);
    m_menu_discord_sub.append(m_menu_discord_set_token);
    m_menu_discord_sub.append(m_menu_discord_join_guild);
    m_menu_discord_sub.append(m_menu_discord_set_status);
    m_menu_discord_sub.append(m_menu_discord_add_recipient);
    m_menu_discord_sub.signal_popped_up().connect(sigc::mem_fun(*this, &MainWindow::OnDiscordSubmenuPopup)); // this gets called twice for some reason
    m_menu_discord.set_submenu(m_menu_discord_sub);

    m_menu_file.set_label("File");
    m_menu_file.set_submenu(m_menu_file_sub);
    m_menu_file_reload_css.set_label("Reload CSS");
    m_menu_file_clear_cache.set_label("Clear file cache");
    m_menu_file_sub.append(m_menu_file_reload_css);
    m_menu_file_sub.append(m_menu_file_clear_cache);

    m_menu_view.set_label("View");
    m_menu_view.set_submenu(m_menu_view_sub);
    m_menu_view_friends.set_label("Friends");
    m_menu_view_pins.set_label("Pins");
    m_menu_view_sub.append(m_menu_view_friends);
    m_menu_view_sub.append(m_menu_view_pins);
    m_menu_view_sub.signal_popped_up().connect(sigc::mem_fun(*this, &MainWindow::OnViewSubmenuPopup));

    m_menu_bar.append(m_menu_file);
    m_menu_bar.append(m_menu_discord);
    m_menu_bar.append(m_menu_view);
    m_menu_bar.show_all();

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

    m_menu_file_clear_cache.signal_activate().connect([this] {
        Abaddon::Get().GetImageManager().ClearCache();
    });

    m_menu_discord_add_recipient.signal_activate().connect([this] {
        m_signal_action_add_recipient.emit(GetChatActiveChannel());
    });

    m_menu_view_friends.signal_activate().connect([this] {
        UpdateChatActiveChannel(Snowflake::Invalid);
        m_members.UpdateMemberList();
        m_content_stack.set_visible_child("friends");
    });

    m_menu_view_pins.signal_activate().connect([this] {
        m_signal_action_view_pins.emit(GetChatActiveChannel());
    });

    m_content_box.set_hexpand(true);
    m_content_box.set_vexpand(true);
    m_content_box.show();

    m_main_box.add(m_menu_bar);
    m_main_box.add(m_content_box);
    m_main_box.show();

    auto *member_list = m_members.GetRoot();
    auto *chat = m_chat.GetRoot();

    chat->set_vexpand(true);
    chat->set_hexpand(true);
    chat->show();

    m_channel_list.set_vexpand(true);
    m_channel_list.set_size_request(-1, -1);
    m_channel_list.show();

    member_list->set_vexpand(true);
    member_list->show();

    m_friends.set_vexpand(true);
    m_friends.set_hexpand(true);
    m_friends.show();

    m_content_stack.add(*chat, "chat");
    m_content_stack.add(m_friends, "friends");
    m_content_stack.set_vexpand(true);
    m_content_stack.set_hexpand(true);
    m_content_stack.set_visible_child("chat");
    m_content_stack.show();

    m_chan_content_paned.pack1(m_channel_list);
    m_chan_content_paned.pack2(m_content_members_paned);
    m_chan_content_paned.child_property_shrink(m_channel_list) = false;
    m_chan_content_paned.child_property_resize(m_channel_list) = false;
    m_chan_content_paned.set_position(200);
    m_chan_content_paned.show();
    m_content_box.add(m_chan_content_paned);

    m_content_members_paned.pack1(m_content_stack);
    m_content_members_paned.pack2(*member_list);
    m_content_members_paned.child_property_shrink(*member_list) = false;
    m_content_members_paned.child_property_resize(*member_list) = false;
    int w, h;
    get_default_size(w, h); // :s
    m_content_members_paned.set_position(w - m_chan_content_paned.get_position() - 150);
    m_content_members_paned.show();

    add(m_main_box);
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
        m_chat.Clear();
        m_members.Clear();
    } else {
        m_members.UpdateMemberList();
    }
    UpdateChannelListing();
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
    auto msgs = discord.GetMessagesForChannel(m_chat.GetActiveChannel(), 50);
    m_chat.SetMessages(msgs);
    m_members.UpdateMemberList();
}

void MainWindow::UpdateChatActiveChannel(Snowflake id) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    m_chat.SetActiveChannel(id);
    m_members.SetActiveChannel(id);
    m_channel_list.SetActiveChannel(id);
    m_content_stack.set_visible_child("chat");
}

Snowflake MainWindow::GetChatActiveChannel() const {
    return m_chat.GetActiveChannel();
}

void MainWindow::UpdateChatNewMessage(const Message &data) {
    if (data.ChannelID == GetChatActiveChannel()) {
        m_chat.AddNewMessage(data);
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

void MainWindow::UpdateChatPrependHistory(const std::vector<Message> &msgs) {
    m_chat.AddNewHistory(msgs); // given vector should be sorted ascending
}

void MainWindow::InsertChatInput(std::string text) {
    m_chat.InsertChatInput(text);
}

Snowflake MainWindow::GetChatOldestListedMessage() {
    return m_chat.GetOldestListedMessage();
}

void MainWindow::UpdateChatReactionAdd(Snowflake id, const Glib::ustring &param) {
    m_chat.UpdateReactions(id);
}

void MainWindow::UpdateChatReactionRemove(Snowflake id, const Glib::ustring &param) {
    m_chat.UpdateReactions(id);
}

void MainWindow::OnDiscordSubmenuPopup(const Gdk::Rectangle *flipped_rect, const Gdk::Rectangle *final_rect, bool flipped_x, bool flipped_y) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto channel_id = GetChatActiveChannel();
    auto channel = discord.GetChannel(channel_id);
    m_menu_discord_add_recipient.set_visible(false);
    if (channel.has_value() && channel->GetDMRecipients().size() + 1 < 10)
        m_menu_discord_add_recipient.set_visible(channel->Type == ChannelType::GROUP_DM);
}

void MainWindow::OnViewSubmenuPopup(const Gdk::Rectangle *flipped_rect, const Gdk::Rectangle *final_rect, bool flipped_x, bool flipped_y) {
    m_menu_view_friends.set_sensitive(Abaddon::Get().GetDiscordClient().IsStarted());
    auto channel_id = GetChatActiveChannel();
    auto channel = Abaddon::Get().GetDiscordClient().GetChannel(channel_id);
    m_menu_view_pins.set_sensitive(false);
    if (channel.has_value())
        m_menu_view_pins.set_sensitive(channel->Type == ChannelType::GUILD_TEXT);
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

MainWindow::type_signal_action_add_recipient MainWindow::signal_action_add_recipient() {
    return m_signal_action_add_recipient;
}

MainWindow::type_signal_action_view_pins MainWindow::signal_action_view_pins() {
    return m_signal_action_view_pins;
}
