#include "mainwindow.hpp"

#include "abaddon.hpp"
#include "util.hpp"

MainWindow::MainWindow()
    : m_main_box(Gtk::ORIENTATION_VERTICAL)
    , m_content_box(Gtk::ORIENTATION_HORIZONTAL)
    , m_chan_content_paned(Gtk::ORIENTATION_HORIZONTAL)
    , m_content_members_paned(Gtk::ORIENTATION_HORIZONTAL)
    , m_left_pane(Gtk::ORIENTATION_VERTICAL)
    , m_accels(Gtk::AccelGroup::create()) {
    set_default_size(1200, 800);
    get_style_context()->add_class("app-window");

    add_accel_group(m_accels);

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

#ifdef WITH_LIBHANDY
    m_channel_list.signal_action_open_new_tab().connect([this](Snowflake id) {
        m_chat.OpenNewTab(id);
    });
#endif

    m_channel_list.SetClassic(Abaddon::Get().GetSettings().ClassicChannels);
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

    m_left_pane.add(m_channel_list);
#ifdef WITH_VOICE
    m_left_pane.add(m_voice_info);
#endif
    m_left_pane.show();

    m_chan_content_paned.pack1(m_left_pane);
    m_chan_content_paned.pack2(m_content_members_paned);
    m_chan_content_paned.child_property_shrink(m_content_members_paned) = true;
    m_chan_content_paned.child_property_resize(m_content_members_paned) = true;
    m_chan_content_paned.child_property_shrink(m_left_pane) = true;
    m_chan_content_paned.child_property_resize(m_left_pane) = true;
    m_chan_content_paned.set_position(200);
    m_chan_content_paned.show();
    m_content_box.add(m_chan_content_paned);
    m_channel_list.UsePanedHack(m_chan_content_paned);

    m_content_members_paned.pack1(m_content_stack);
    m_content_members_paned.pack2(*member_list);
    m_content_members_paned.child_property_shrink(m_content_stack) = true;
    m_content_members_paned.child_property_resize(m_content_stack) = true;
    m_content_members_paned.child_property_shrink(*member_list) = true;
    m_content_members_paned.child_property_resize(*member_list) = true;
    int w, h;
    get_default_size(w, h); // :s
    m_content_members_paned.set_position(w - m_chan_content_paned.get_position() - 150);
    m_content_members_paned.show();

    add(m_main_box);

    SetupMenu();
    SetupDND();
}

void MainWindow::UpdateComponents() {
    bool discord_active = Abaddon::Get().IsDiscordActive();

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

void MainWindow::UpdateChatWindowContents() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto msgs = discord.GetMessagesForChannel(m_chat.GetActiveChannel(), 50);
    m_chat.SetMessages(msgs);
    m_members.UpdateMemberList();
}

void MainWindow::UpdateChatActiveChannel(Snowflake id, bool expand_to) {
    m_chat.SetActiveChannel(id);
    m_members.SetActiveChannel(id);
    m_channel_list.SetActiveChannel(id, expand_to);
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

void MainWindow::InsertChatInput(const std::string &text) {
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

void MainWindow::UpdateMenus() {
    OnDiscordSubmenuPopup();
    OnViewSubmenuPopup();
}

void MainWindow::ToggleMenuVisibility() {
    m_menu_bar.set_visible(!m_menu_bar.get_visible());
}

void MainWindow::EditMessage(Snowflake message_id) {
    m_chat.StartEditing(message_id);
}

#ifdef WITH_LIBHANDY
void MainWindow::GoBack() {
    m_chat.GoBack();
}

void MainWindow::GoForward() {
    m_chat.GoForward();
}

void MainWindow::GoToPreviousTab() {
    m_chat.GoToPreviousTab();
}

void MainWindow::GoToNextTab() {
    m_chat.GoToNextTab();
}

void MainWindow::GoToTab(int idx) {
    m_chat.GoToTab(idx);
}
#endif

void MainWindow::OnDiscordSubmenuPopup() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto channel_id = GetChatActiveChannel();
    m_menu_discord_add_recipient.set_visible(false);
    if (channel_id.IsValid()) {
        auto channel = discord.GetChannel(channel_id);
        if (channel.has_value() && channel->GetDMRecipients().size() + 1 < 10)
            m_menu_discord_add_recipient.set_visible(channel->Type == ChannelType::GROUP_DM);
    }

    const bool discord_active = Abaddon::Get().GetDiscordClient().IsStarted();

    std::string token = Abaddon::Get().GetDiscordToken();
    m_menu_discord_connect.set_sensitive(!token.empty() && !discord_active);
    m_menu_discord_disconnect.set_sensitive(discord_active);
    m_menu_discord_set_token.set_sensitive(!discord_active);
#ifdef WITH_QRLOGIN
    m_menu_discord_login_qr.set_sensitive(!discord_active);
#endif
    m_menu_discord_set_status.set_sensitive(discord_active);
}

void MainWindow::OnViewSubmenuPopup() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const bool discord_active = discord.IsStarted();

    m_menu_view_friends.set_sensitive(discord_active);
    m_menu_view_mark_guild_as_read.set_sensitive(discord_active);

    auto channel_id = GetChatActiveChannel();
    m_menu_view_pins.set_sensitive(false);
    m_menu_view_threads.set_sensitive(false);
    if (channel_id.IsValid()) {
        if (auto channel = discord.GetChannel(channel_id); channel.has_value()) {
            m_menu_view_threads.set_sensitive(channel->Type == ChannelType::GUILD_TEXT || channel->IsThread());
            m_menu_view_pins.set_sensitive(channel->Type == ChannelType::GUILD_TEXT || channel->Type == ChannelType::DM || channel->Type == ChannelType::GROUP_DM || channel->IsThread());
        }
    }
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

void MainWindow::SetupMenu() {
    m_menu_discord.set_label("Discord");
    m_menu_discord.set_submenu(m_menu_discord_sub);
    m_menu_discord_connect.set_label("Connect");
    m_menu_discord_connect.set_sensitive(false);
    m_menu_discord_disconnect.set_label("Disconnect");
    m_menu_discord_disconnect.set_sensitive(false);
    m_menu_discord_set_token.set_label("Set Token");
    m_menu_discord_login_qr.set_label("Login with QR Code");
#ifndef WITH_QRLOGIN
    m_menu_discord_login_qr.set_sensitive(false);
    m_menu_discord_login_qr.set_tooltip_text("Not compiled with support");
#endif
    m_menu_discord_set_status.set_label("Set Status");
    m_menu_discord_set_status.set_sensitive(false);
    m_menu_discord_add_recipient.set_label("Add user to DM");
    m_menu_discord_sub.append(m_menu_discord_connect);
    m_menu_discord_sub.append(m_menu_discord_disconnect);
    m_menu_discord_sub.append(m_menu_discord_set_token);
    m_menu_discord_sub.append(m_menu_discord_login_qr);
    m_menu_discord_sub.append(m_menu_discord_set_status);
    m_menu_discord_sub.append(m_menu_discord_add_recipient);
    m_menu_discord.set_submenu(m_menu_discord_sub);

    m_menu_file.set_label("File");
    m_menu_file.set_submenu(m_menu_file_sub);
    m_menu_file_reload_css.set_label("Reload CSS");
    m_menu_file_clear_cache.set_label("Clear file cache");
    m_menu_file_dump_ready.set_label("Dump ready message");
    m_menu_file_sub.append(m_menu_file_reload_css);
    m_menu_file_sub.append(m_menu_file_clear_cache);
    m_menu_file_sub.append(m_menu_file_dump_ready);

    m_menu_view.set_label("View");
    m_menu_view.set_submenu(m_menu_view_sub);
    m_menu_view_friends.set_label("Friends");
    m_menu_view_pins.set_label("Pins");
    m_menu_view_threads.set_label("Threads");
    m_menu_view_mark_guild_as_read.set_label("Mark Server as Read");
    m_menu_view_mark_guild_as_read.add_accelerator("activate", m_accels, GDK_KEY_Escape, Gdk::SHIFT_MASK, Gtk::ACCEL_VISIBLE);
    m_menu_view_channels.set_label("Channels");
    m_menu_view_channels.add_accelerator("activate", m_accels, GDK_KEY_L, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    m_menu_view_channels.set_active(true);
    m_menu_view_members.set_label("Members");
    m_menu_view_members.add_accelerator("activate", m_accels, GDK_KEY_M, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    m_menu_view_members.set_active(true);
#ifdef WITH_LIBHANDY
    m_menu_view_go_back.set_label("Go Back");
    m_menu_view_go_forward.set_label("Go Forward");
    m_menu_view_go_back.add_accelerator("activate", m_accels, GDK_KEY_Left, Gdk::MOD1_MASK, Gtk::ACCEL_VISIBLE);
    m_menu_view_go_forward.add_accelerator("activate", m_accels, GDK_KEY_Right, Gdk::MOD1_MASK, Gtk::ACCEL_VISIBLE);
#endif
    m_menu_view_sub.append(m_menu_view_friends);
    m_menu_view_sub.append(m_menu_view_pins);
    m_menu_view_sub.append(m_menu_view_threads);
    m_menu_view_sub.append(m_menu_view_mark_guild_as_read);
    m_menu_view_sub.append(m_menu_view_channels);
    m_menu_view_sub.append(m_menu_view_members);
#ifdef WITH_LIBHANDY
    m_menu_view_sub.append(m_menu_view_go_back);
    m_menu_view_sub.append(m_menu_view_go_forward);
#endif

    m_menu_bar.append(m_menu_file);
    m_menu_bar.append(m_menu_discord);
    m_menu_bar.append(m_menu_view);

    if (Abaddon::Get().GetSettings().AltMenu) {
        auto set_hide_cb = [this](Gtk::Menu &menu) {
            for (auto *child : menu.get_children()) {
                auto *item = dynamic_cast<Gtk::MenuItem *>(child);
                if (item != nullptr) {
                    item->signal_activate().connect([this]() {
                        m_menu_bar.hide();
                    });
                }
            }
        };
        set_hide_cb(m_menu_discord_sub);
        set_hide_cb(m_menu_file_sub);
        set_hide_cb(m_menu_view_sub);
        m_menu_bar.show_all_children();
    } else {
        m_menu_bar.show_all();
    }

    m_menu_discord_connect.signal_activate().connect([this] {
        m_signal_action_connect.emit();
    });

    m_menu_discord_disconnect.signal_activate().connect([this] {
        m_signal_action_disconnect.emit();
    });

    m_menu_discord_set_token.signal_activate().connect([this] {
        m_signal_action_set_token.emit();
    });

    m_menu_discord_login_qr.signal_activate().connect([this] {
        m_signal_action_login_qr.emit();
    });

    m_menu_file_reload_css.signal_activate().connect([this] {
        m_signal_action_reload_css.emit();
    });

    m_menu_discord_set_status.signal_activate().connect([this] {
        m_signal_action_set_status.emit();
    });

    m_menu_file_clear_cache.signal_activate().connect([] {
        Abaddon::Get().GetImageManager().ClearCache();
    });

    m_menu_file_dump_ready.signal_toggled().connect([this]() {
        Abaddon::Get().GetDiscordClient().SetDumpReady(m_menu_file_dump_ready.get_active());
    });

    m_menu_discord_add_recipient.signal_activate().connect([this] {
        m_signal_action_add_recipient.emit(GetChatActiveChannel());
    });

    m_menu_view_friends.signal_activate().connect([this] {
        UpdateChatActiveChannel(Snowflake::Invalid, true);
        m_members.UpdateMemberList();
        m_content_stack.set_visible_child("friends");
    });

    m_menu_view_pins.signal_activate().connect([this] {
        m_signal_action_view_pins.emit(GetChatActiveChannel());
    });

    m_menu_view_threads.signal_activate().connect([this] {
        m_signal_action_view_threads.emit(GetChatActiveChannel());
    });

    m_menu_view_mark_guild_as_read.signal_activate().connect([this] {
        auto &discord = Abaddon::Get().GetDiscordClient();
        const auto channel_id = GetChatActiveChannel();
        const auto channel = discord.GetChannel(channel_id);
        if (channel.has_value() && channel->GuildID.has_value()) {
            discord.MarkGuildAsRead(*channel->GuildID, NOOP_CALLBACK);
        }
    });

    m_menu_view_channels.signal_activate().connect([this]() {
        m_left_pane.set_visible(m_menu_view_channels.get_active());
    });

    m_menu_view_members.signal_activate().connect([this]() {
        m_members.GetRoot()->set_visible(m_menu_view_members.get_active());
    });

#ifdef WITH_LIBHANDY
    m_menu_view_go_back.signal_activate().connect([this] {
        GoBack();
    });

    m_menu_view_go_forward.signal_activate().connect([this] {
        GoForward();
    });
#endif
}

void MainWindow::SetupDND() {
    std::vector<Gtk::TargetEntry> targets;
    targets.emplace_back("text/uri-list", Gtk::TargetFlags(0), 0);
    drag_dest_set(targets, Gtk::DEST_DEFAULT_DROP | Gtk::DEST_DEFAULT_MOTION | Gtk::DEST_DEFAULT_HIGHLIGHT, Gdk::DragAction::ACTION_COPY);
    signal_drag_data_received().connect([this](const Glib::RefPtr<Gdk::DragContext> &ctx, int x, int y, const Gtk::SelectionData &selection, guint info, guint time) {
        HandleDroppedURIs(selection);
    });
}

void MainWindow::HandleDroppedURIs(const Gtk::SelectionData &selection) {
    for (const auto &uri : selection.get_uris()) {
        // not using Glib::get_filename_for_uri or whatever because the conversion is BAD (on windows at least)
        m_chat.AddAttachment(Gio::File::create_for_uri(uri));
    }
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

MainWindow::type_signal_action_login_qr MainWindow::signal_action_login_qr() {
    return m_signal_action_login_qr;
}

MainWindow::type_signal_action_reload_css MainWindow::signal_action_reload_css() {
    return m_signal_action_reload_css;
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

MainWindow::type_signal_action_view_threads MainWindow::signal_action_view_threads() {
    return m_signal_action_view_threads;
}