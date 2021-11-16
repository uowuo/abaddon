#include <gtkmm.h>
#include <memory>
#include <string>
#include <algorithm>
#include "platform.hpp"
#include "discord/discord.hpp"
#include "dialogs/token.hpp"
#include "dialogs/editmessage.hpp"
#include "dialogs/joinguild.hpp"
#include "dialogs/confirm.hpp"
#include "dialogs/setstatus.hpp"
#include "dialogs/friendpicker.hpp"
#include "dialogs/verificationgate.hpp"
#include "abaddon.hpp"
#include "windows/guildsettingswindow.hpp"
#include "windows/profilewindow.hpp"
#include "windows/pinnedwindow.hpp"
#include "windows/threadswindow.hpp"

#ifdef _WIN32
    #pragma comment(lib, "crypt32.lib")
#endif

Abaddon::Abaddon()
    : m_settings(Platform::FindConfigFile())
    , m_discord(m_settings.GetUseMemoryDB()) // stupid but easy
    , m_emojis(GetResPath("/emojis.bin")) {
    LoadFromSettings();

    // todo: set user agent for non-client(?)
    std::string ua = m_settings.GetUserAgent();
    m_discord.SetUserAgent(ua);

    m_discord.signal_gateway_ready().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnReady));
    m_discord.signal_message_create().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageCreate));
    m_discord.signal_message_delete().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageDelete));
    m_discord.signal_message_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageUpdate));
    m_discord.signal_guild_member_list_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildMemberListUpdate));
    m_discord.signal_thread_member_list_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnThreadMemberListUpdate));
    m_discord.signal_reaction_add().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnReactionAdd));
    m_discord.signal_reaction_remove().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnReactionRemove));
    m_discord.signal_guild_join_request_create().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildJoinRequestCreate));
    m_discord.signal_thread_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnThreadUpdate));
    m_discord.signal_message_sent().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageSent));
    m_discord.signal_disconnected().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnDisconnect));
    if (m_settings.GetPrefetch())
        m_discord.signal_message_create().connect([this](const Message &message) {
            if (message.Author.HasAvatar())
                m_img_mgr.Prefetch(message.Author.GetAvatarURL());
            for (const auto &attachment : message.Attachments) {
                if (IsURLViewableImage(attachment.ProxyURL))
                    m_img_mgr.Prefetch(attachment.ProxyURL);
            }
        });
}

Abaddon::~Abaddon() {
    m_settings.Close();
}

Abaddon &Abaddon::Get() {
    static Abaddon instance;
    return instance;
}

int Abaddon::StartGTK() {
    m_gtk_app = Gtk::Application::create("com.github.uowuo.abaddon");

    m_css_provider = Gtk::CssProvider::create();
    m_css_provider->signal_parsing_error().connect([this](const Glib::RefPtr<const Gtk::CssSection> &section, const Glib::Error &error) {
        Gtk::MessageDialog dlg(*m_main_window, "css failed parsing (" + error.what() + ")", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    });

    m_css_low_provider = Gtk::CssProvider::create();
    m_css_low_provider->signal_parsing_error().connect([this](const Glib::RefPtr<const Gtk::CssSection> &section, const Glib::Error &error) {
        Gtk::MessageDialog dlg(*m_main_window, "low-priority css failed parsing (" + error.what() + ")", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    });

    m_main_window = std::make_unique<MainWindow>();
    m_main_window->set_title(APP_TITLE);
    m_main_window->UpdateComponents();
    m_main_window->set_position(Gtk::WIN_POS_CENTER);

    // crashes for some stupid reason if i put it somewhere else
    SetupUserMenu();

    m_main_window->signal_action_connect().connect(sigc::mem_fun(*this, &Abaddon::ActionConnect));
    m_main_window->signal_action_disconnect().connect(sigc::mem_fun(*this, &Abaddon::ActionDisconnect));
    m_main_window->signal_action_set_token().connect(sigc::mem_fun(*this, &Abaddon::ActionSetToken));
    m_main_window->signal_action_reload_css().connect(sigc::mem_fun(*this, &Abaddon::ActionReloadCSS));
    m_main_window->signal_action_join_guild().connect(sigc::mem_fun(*this, &Abaddon::ActionJoinGuildDialog));
    m_main_window->signal_action_set_status().connect(sigc::mem_fun(*this, &Abaddon::ActionSetStatus));
    m_main_window->signal_action_add_recipient().connect(sigc::mem_fun(*this, &Abaddon::ActionAddRecipient));
    m_main_window->signal_action_view_pins().connect(sigc::mem_fun(*this, &Abaddon::ActionViewPins));
    m_main_window->signal_action_view_threads().connect(sigc::mem_fun(*this, &Abaddon::ActionViewThreads));

    m_main_window->GetChannelList()->signal_action_channel_item_select().connect(sigc::mem_fun(*this, &Abaddon::ActionChannelOpened));
    m_main_window->GetChannelList()->signal_action_guild_leave().connect(sigc::mem_fun(*this, &Abaddon::ActionLeaveGuild));
    m_main_window->GetChannelList()->signal_action_guild_settings().connect(sigc::mem_fun(*this, &Abaddon::ActionGuildSettings));

    m_main_window->GetChatWindow()->signal_action_message_edit().connect(sigc::mem_fun(*this, &Abaddon::ActionChatEditMessage));
    m_main_window->GetChatWindow()->signal_action_chat_submit().connect(sigc::mem_fun(*this, &Abaddon::ActionChatInputSubmit));
    m_main_window->GetChatWindow()->signal_action_chat_load_history().connect(sigc::mem_fun(*this, &Abaddon::ActionChatLoadHistory));
    m_main_window->GetChatWindow()->signal_action_channel_click().connect(sigc::mem_fun(*this, &Abaddon::ActionChannelOpened));
    m_main_window->GetChatWindow()->signal_action_insert_mention().connect(sigc::mem_fun(*this, &Abaddon::ActionInsertMention));
    m_main_window->GetChatWindow()->signal_action_reaction_add().connect(sigc::mem_fun(*this, &Abaddon::ActionReactionAdd));
    m_main_window->GetChatWindow()->signal_action_reaction_remove().connect(sigc::mem_fun(*this, &Abaddon::ActionReactionRemove));

    ActionReloadCSS();

    m_gtk_app->signal_shutdown().connect(sigc::mem_fun(*this, &Abaddon::StopDiscord), false);

    if (!m_settings.IsValid()) {
        Gtk::MessageDialog dlg(*m_main_window, "The settings file could not be created!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }

    if (!m_emojis.Load()) {
        Gtk::MessageDialog dlg(*m_main_window, "The emoji file couldn't be loaded!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }

    if (!m_discord.IsStoreValid()) {
        Gtk::MessageDialog dlg(*m_main_window, "The Discord cache could not be created!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
        return 1;
    }

    m_main_window->show();
    return m_gtk_app->run(*m_main_window);
}

void Abaddon::LoadFromSettings() {
    std::string token = m_settings.GetDiscordToken();
    if (token.size()) {
        m_discord_token = token;
        m_discord.UpdateToken(m_discord_token);
    }
}

void Abaddon::StartDiscord() {
    m_discord.Start();
}

void Abaddon::StopDiscord() {
    m_discord.Stop();
    SaveState();
}

bool Abaddon::IsDiscordActive() const {
    return m_discord.IsStarted();
}

std::string Abaddon::GetDiscordToken() const {
    return m_discord_token;
}

DiscordClient &Abaddon::GetDiscordClient() {
    std::scoped_lock<std::mutex> guard(m_mutex);
    return m_discord;
}

const DiscordClient &Abaddon::GetDiscordClient() const {
    std::scoped_lock<std::mutex> guard(m_mutex);
    return m_discord;
}

void Abaddon::DiscordOnReady() {
    m_main_window->UpdateComponents();
    LoadState();
}

void Abaddon::DiscordOnMessageCreate(const Message &message) {
    m_main_window->UpdateChatNewMessage(message);
}

void Abaddon::DiscordOnMessageDelete(Snowflake id, Snowflake channel_id) {
    m_main_window->UpdateChatMessageDeleted(id, channel_id);
}

void Abaddon::DiscordOnMessageUpdate(Snowflake id, Snowflake channel_id) {
    m_main_window->UpdateChatMessageUpdated(id, channel_id);
}

void Abaddon::DiscordOnGuildMemberListUpdate(Snowflake guild_id) {
    m_main_window->UpdateMembers();
}

void Abaddon::DiscordOnThreadMemberListUpdate(const ThreadMemberListUpdateData &data) {
    m_main_window->UpdateMembers();
}

void Abaddon::DiscordOnReactionAdd(Snowflake message_id, const Glib::ustring &param) {
    m_main_window->UpdateChatReactionAdd(message_id, param);
}

void Abaddon::DiscordOnReactionRemove(Snowflake message_id, const Glib::ustring &param) {
    m_main_window->UpdateChatReactionAdd(message_id, param);
}

// this will probably cause issues when actual applications are rolled out but that doesn't seem like it will happen for a while
void Abaddon::DiscordOnGuildJoinRequestCreate(const GuildJoinRequestCreateData &data) {
    if (data.Status == GuildApplicationStatus::STARTED) {
        ShowGuildVerificationGateDialog(data.GuildID);
    }
}

void Abaddon::DiscordOnMessageSent(const Message &data) {
    m_main_window->UpdateChatNewMessage(data);
}

void Abaddon::DiscordOnDisconnect(bool is_reconnecting, GatewayCloseCode close_code) {
    m_channels_history_loaded.clear();
    m_channels_history_loading.clear();
    m_channels_requested.clear();
    if (is_reconnecting) return;
    m_main_window->set_title(APP_TITLE);
    m_main_window->UpdateComponents();

    if (close_code == GatewayCloseCode::AuthenticationFailed) {
        Gtk::MessageDialog dlg(*m_main_window, "Discord rejected your token", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    } else if (close_code != GatewayCloseCode::Normal) {
        Gtk::MessageDialog dlg(*m_main_window,
                               "Lost connection with Discord's gateway. Try reconnecting (code " + std::to_string(static_cast<unsigned>(close_code)) + ")",
                               false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }
}

void Abaddon::DiscordOnThreadUpdate(const ThreadUpdateData &data) {
    if (data.Thread.ID == m_main_window->GetChatActiveChannel()) {
        if (data.Thread.ThreadMetadata->IsArchived)
            m_main_window->GetChatWindow()->SetTopic("This thread is archived. Sending a message will unarchive it");
        else
            m_main_window->GetChatWindow()->SetTopic("");
    }
}

const SettingsManager &Abaddon::GetSettings() const {
    return m_settings;
}

Glib::RefPtr<Gtk::CssProvider> Abaddon::GetStyleProvider() {
    return m_css_provider;
}

void Abaddon::ShowUserMenu(const GdkEvent *event, Snowflake id, Snowflake guild_id) {
    m_shown_user_menu_id = id;
    m_shown_user_menu_guild_id = guild_id;

    const auto guild = m_discord.GetGuild(guild_id);
    const auto me = m_discord.GetUserData().ID;
    const auto user = m_discord.GetMember(id, guild_id);

    for (const auto child : m_user_menu_roles_submenu->get_children())
        delete child;
    if (guild.has_value() && user.has_value()) {
        const auto roles = user->GetSortedRoles();
        m_user_menu_roles->set_visible(roles.size() > 0);
        for (const auto &role : roles) {
            auto *item = Gtk::manage(new Gtk::MenuItem(role.Name));
            if (role.Color != 0) {
                Gdk::RGBA color;
                color.set_red(((role.Color & 0xFF0000) >> 16) / 255.0);
                color.set_green(((role.Color & 0x00FF00) >> 8) / 255.0);
                color.set_blue(((role.Color & 0x0000FF) >> 0) / 255.0);
                color.set_alpha(1.0);
                item->override_color(color);
            }
            item->show();
            m_user_menu_roles_submenu->append(*item);
        }
    } else
        m_user_menu_roles->set_visible(false);

    if (me == id) {
        m_user_menu_ban->set_visible(false);
        m_user_menu_kick->set_visible(false);
        m_user_menu_open_dm->set_visible(false);
    } else {
        const bool has_kick = m_discord.HasGuildPermission(me, guild_id, Permission::KICK_MEMBERS);
        const bool has_ban = m_discord.HasGuildPermission(me, guild_id, Permission::BAN_MEMBERS);
        const bool can_manage = m_discord.CanManageMember(guild_id, me, id);

        m_user_menu_kick->set_visible(has_kick && can_manage);
        m_user_menu_ban->set_visible(has_ban && can_manage);
        m_user_menu_open_dm->set_visible(true);
    }

    m_user_menu_remove_recipient->hide();
    if (me != id) {
        const auto channel_id = m_main_window->GetChatActiveChannel();
        const auto channel = m_discord.GetChannel(channel_id);
        if (channel.has_value() && channel->Type == ChannelType::GROUP_DM && me == *channel->OwnerID)
            m_user_menu_remove_recipient->show();
    }

    m_user_menu->popup_at_pointer(event);
}

void Abaddon::ShowGuildVerificationGateDialog(Snowflake guild_id) {
    VerificationGateDialog dlg(*m_main_window, guild_id);
    if (dlg.run() == Gtk::RESPONSE_OK) {
        const auto cb = [this](DiscordError code) {
            if (code != DiscordError::NONE) {
                Gtk::MessageDialog dlg(*m_main_window, "Failed to accept the verification gate.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
                dlg.set_position(Gtk::WIN_POS_CENTER);
                dlg.run();
            }
        };
        m_discord.AcceptVerificationGate(guild_id, dlg.GetVerificationGate(), cb);
    }
}

void Abaddon::SetupUserMenu() {
    m_user_menu = Gtk::manage(new Gtk::Menu);
    m_user_menu_insert_mention = Gtk::manage(new Gtk::MenuItem("Insert Mention"));
    m_user_menu_ban = Gtk::manage(new Gtk::MenuItem("Ban"));
    m_user_menu_kick = Gtk::manage(new Gtk::MenuItem("Kick"));
    m_user_menu_copy_id = Gtk::manage(new Gtk::MenuItem("Copy ID"));
    m_user_menu_open_dm = Gtk::manage(new Gtk::MenuItem("Open DM"));
    m_user_menu_roles = Gtk::manage(new Gtk::MenuItem("Roles"));
    m_user_menu_info = Gtk::manage(new Gtk::MenuItem("View Profile"));
    m_user_menu_remove_recipient = Gtk::manage(new Gtk::MenuItem("Remove From Group"));
    m_user_menu_roles_submenu = Gtk::manage(new Gtk::Menu);
    m_user_menu_roles->set_submenu(*m_user_menu_roles_submenu);
    m_user_menu_insert_mention->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_insert_mention));
    m_user_menu_ban->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_ban));
    m_user_menu_kick->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_kick));
    m_user_menu_copy_id->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_copy_id));
    m_user_menu_open_dm->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_open_dm));
    m_user_menu_remove_recipient->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_remove_recipient));
    m_user_menu_info->signal_activate().connect([this]() {
        auto *window = new ProfileWindow(m_shown_user_menu_id);
        ManageHeapWindow(window);
        window->show();
    });

    m_user_menu_remove_recipient->override_color(Gdk::RGBA("#BE3C3D"));

    m_user_menu->append(*m_user_menu_info);
    m_user_menu->append(*m_user_menu_insert_mention);
    m_user_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem));
    m_user_menu->append(*m_user_menu_ban);
    m_user_menu->append(*m_user_menu_kick);
    m_user_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem));
    m_user_menu->append(*m_user_menu_open_dm);
    m_user_menu->append(*m_user_menu_roles);
    m_user_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem));
    m_user_menu->append(*m_user_menu_remove_recipient);
    m_user_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem));
    m_user_menu->append(*m_user_menu_copy_id);

    m_user_menu->show_all();
}

void Abaddon::SaveState() {
    if (!m_settings.GetSaveState()) return;

    AbaddonApplicationState state;
    state.ActiveChannel = m_main_window->GetChatActiveChannel();
    state.Expansion = m_main_window->GetChannelList()->GetExpansionState();

    const auto path = GetStateCachePath();
    if (!util::IsFolder(path)) {
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
    }

    auto *fp = std::fopen(GetStateCachePath("/state.json").c_str(), "wb");
    if (fp == nullptr) return;
    const auto s = nlohmann::json(state).dump(4);
    std::fwrite(s.c_str(), 1, s.size(), fp);
    std::fclose(fp);
}

void Abaddon::LoadState() {
    if (!m_settings.GetSaveState()) return;

    const auto data = ReadWholeFile(GetStateCachePath("/state.json"));
    if (data.empty()) return;
    try {
        AbaddonApplicationState state = nlohmann::json::parse(data.begin(), data.end());
        m_main_window->GetChannelList()->UseExpansionState(state.Expansion);
        ActionChannelOpened(state.ActiveChannel);
    } catch (const std::exception &e) {
        printf("failed to load application state: %s\n", e.what());
    }
}

void Abaddon::ManageHeapWindow(Gtk::Window *window) {
    window->signal_hide().connect([this, window]() {
        delete window;
        // for some reason if ShowUserMenu is called multiple times with events across windows
        // and one of the windows is closed, then it throws errors when the menu is opened again
        // i dont think this is my fault so this semi-hacky solution will have to do
        delete m_user_menu;
        SetupUserMenu();
    });
}

void Abaddon::on_user_menu_insert_mention() {
    ActionInsertMention(m_shown_user_menu_id);
}

void Abaddon::on_user_menu_ban() {
    ActionBanMember(m_shown_user_menu_id, m_shown_user_menu_guild_id);
}

void Abaddon::on_user_menu_kick() {
    ActionKickMember(m_shown_user_menu_id, m_shown_user_menu_guild_id);
}

void Abaddon::on_user_menu_copy_id() {
    Gtk::Clipboard::get()->set_text(std::to_string(m_shown_user_menu_id));
}

void Abaddon::on_user_menu_open_dm() {
    const auto existing = m_discord.FindDM(m_shown_user_menu_id);
    if (existing.has_value())
        ActionChannelOpened(*existing);
    else
        m_discord.CreateDM(m_shown_user_menu_id, [this](DiscordError code, Snowflake channel_id) {
            if (code == DiscordError::NONE) {
                // give the gateway a little window to send CHANNEL_CREATE
                auto cb = [this, channel_id] {
                    ActionChannelOpened(channel_id);
                };
                Glib::signal_timeout().connect_once(sigc::track_obj(cb, *this), 200);
            }
        });
}

void Abaddon::on_user_menu_remove_recipient() {
    m_discord.RemoveGroupDMRecipient(m_main_window->GetChatActiveChannel(), m_shown_user_menu_id);
}

std::string Abaddon::GetCSSPath() {
    const static auto path = Platform::FindResourceFolder() + "/css";
    return path;
}

std::string Abaddon::GetResPath() {
    const static auto path = Platform::FindResourceFolder() + "/res";
    return path;
}

std::string Abaddon::GetStateCachePath() {
    const static auto path = Platform::FindStateCacheFolder() + "/state";
    return path;
}

std::string Abaddon::GetCSSPath(const std::string &path) {
    return GetCSSPath() + path;
}

std::string Abaddon::GetResPath(const std::string &path) {
    return GetResPath() + path;
}

std::string Abaddon::GetStateCachePath(const std::string &path) {
    return GetStateCachePath() + path;
}

void Abaddon::ActionConnect() {
    if (!m_discord.IsStarted())
        StartDiscord();
    m_main_window->UpdateComponents();
}

void Abaddon::ActionDisconnect() {
    StopDiscord();
}

void Abaddon::ActionSetToken() {
    TokenDialog dlg(*m_main_window);
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK) {
        m_discord_token = dlg.GetToken();
        m_discord.UpdateToken(m_discord_token);
        m_main_window->UpdateComponents();
        m_settings.SetSetting("discord", "token", m_discord_token);
    }
}

void Abaddon::ActionJoinGuildDialog() {
    JoinGuildDialog dlg(*m_main_window);
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK) {
        auto code = dlg.GetCode();
        m_discord.JoinGuild(code);
    }
}

void Abaddon::ActionChannelOpened(Snowflake id) {
    if (!id.IsValid() || id == m_main_window->GetChatActiveChannel()) return;

    m_main_window->GetChatWindow()->SetTopic("");

    const auto channel = m_discord.GetChannel(id);
    if (!channel.has_value()) return;
    if (channel->Type == ChannelType::GUILD_TEXT || channel->Type == ChannelType::GUILD_NEWS)
        m_main_window->set_title(std::string(APP_TITLE) + " - #" + *channel->Name);
    else {
        std::string display;
        const auto recipients = channel->GetDMRecipients();
        if (recipients.size() > 1)
            display = std::to_string(recipients.size()) + " users";
        else if (recipients.size() == 1)
            display = recipients[0].Username;
        else
            display = "Empty group";
        m_main_window->set_title(std::string(APP_TITLE) + " - " + display);
    }
    m_main_window->UpdateChatActiveChannel(id);
    if (m_channels_requested.find(id) == m_channels_requested.end()) {
        m_discord.FetchMessagesInChannel(id, [this, id](const std::vector<Message> &msgs) {
            m_main_window->UpdateChatWindowContents();
            m_channels_requested.insert(id);
        });
    } else {
        m_main_window->UpdateChatWindowContents();
    }

    if (channel->IsThread()) {
        m_discord.SendThreadLazyLoad(id);
        if (channel->ThreadMetadata->IsArchived)
            m_main_window->GetChatWindow()->SetTopic("This thread is archived. Sending a message will unarchive it");
    } else if (channel->Type != ChannelType::DM && channel->Type != ChannelType::GROUP_DM && channel->GuildID.has_value()) {
        m_discord.SendLazyLoad(id);

        if (m_discord.IsVerificationRequired(*channel->GuildID))
            ShowGuildVerificationGateDialog(*channel->GuildID);
    }
}

void Abaddon::ActionChatLoadHistory(Snowflake id) {
    if (m_channels_history_loaded.find(id) != m_channels_history_loaded.end())
        return;

    if (m_channels_history_loading.find(id) != m_channels_history_loading.end())
        return;

    Snowflake before_id = m_main_window->GetChatOldestListedMessage();
    auto knownset = m_discord.GetMessageIDsForChannel(id);
    std::vector<Snowflake> knownvec(knownset.begin(), knownset.end());
    std::sort(knownvec.begin(), knownvec.end());
    auto latest = std::find_if(knownvec.begin(), knownvec.end(), [&before_id](Snowflake x) -> bool { return x == before_id; });
    int distance = std::distance(knownvec.begin(), latest);

    if (distance >= 50) {
        std::vector<Message> msgs;
        for (auto it = knownvec.begin() + distance - 50; it != knownvec.begin() + distance; it++)
            msgs.push_back(*m_discord.GetMessage(*it));
        m_main_window->UpdateChatPrependHistory(msgs);
        return;
    }

    m_channels_history_loading.insert(id);

    m_discord.FetchMessagesInChannelBefore(id, before_id, [this, id](const std::vector<Message> &msgs) {
        m_channels_history_loading.erase(id);

        if (msgs.size() == 0) {
            m_channels_history_loaded.insert(id);
        } else {
            m_main_window->UpdateChatPrependHistory(msgs);
        }
    });
}

void Abaddon::ActionChatInputSubmit(std::string msg, Snowflake channel, Snowflake referenced_message) {
    if (msg.substr(0, 7) == "/shrug " || msg == "/shrug")
        msg = msg.substr(6) + "\xC2\xAF\x5C\x5F\x28\xE3\x83\x84\x29\x5F\x2F\xC2\xAF"; // this is important
    if (referenced_message.IsValid())
        m_discord.SendChatMessage(msg, channel, referenced_message);
    else
        m_discord.SendChatMessage(msg, channel);
}

void Abaddon::ActionChatEditMessage(Snowflake channel_id, Snowflake id) {
    const auto msg = m_discord.GetMessage(id);
    if (!msg.has_value()) return;
    EditMessageDialog dlg(*m_main_window);
    dlg.SetContent(msg->Content);
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK) {
        auto new_content = dlg.GetContent();
        m_discord.EditMessage(channel_id, id, new_content);
    }
}

void Abaddon::ActionInsertMention(Snowflake id) {
    m_main_window->InsertChatInput("<@" + std::to_string(id) + ">");
}

void Abaddon::ActionLeaveGuild(Snowflake id) {
    ConfirmDialog dlg(*m_main_window);
    const auto guild = m_discord.GetGuild(id);
    if (guild.has_value())
        dlg.SetConfirmText("Are you sure you want to leave " + guild->Name + "?");
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK)
        m_discord.LeaveGuild(id);
}

void Abaddon::ActionKickMember(Snowflake user_id, Snowflake guild_id) {
    ConfirmDialog dlg(*m_main_window);
    const auto user = m_discord.GetUser(user_id);
    if (user.has_value())
        dlg.SetConfirmText("Are you sure you want to kick " + user->Username + "#" + user->Discriminator + "?");
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK)
        m_discord.KickUser(user_id, guild_id);
}

void Abaddon::ActionBanMember(Snowflake user_id, Snowflake guild_id) {
    ConfirmDialog dlg(*m_main_window);
    const auto user = m_discord.GetUser(user_id);
    if (user.has_value())
        dlg.SetConfirmText("Are you sure you want to ban " + user->Username + "#" + user->Discriminator + "?");
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK)
        m_discord.BanUser(user_id, guild_id);
}

void Abaddon::ActionSetStatus() {
    SetStatusDialog dlg(*m_main_window);
    const auto response = dlg.run();
    if (response != Gtk::RESPONSE_OK || !m_discord.IsStarted()) return;
    const auto status = dlg.GetStatusType();
    const auto activity_type = dlg.GetActivityType();
    const auto activity_name = dlg.GetActivityName();
    if (activity_name == "") {
        m_discord.UpdateStatus(status, false);
    } else {
        ActivityData activity;
        activity.Name = activity_name;
        activity.Type = activity_type;
        m_discord.UpdateStatus(status, false, activity);
    }
}

void Abaddon::ActionReactionAdd(Snowflake id, const Glib::ustring &param) {
    m_discord.AddReaction(id, param);
}

void Abaddon::ActionReactionRemove(Snowflake id, const Glib::ustring &param) {
    m_discord.RemoveReaction(id, param);
}

void Abaddon::ActionGuildSettings(Snowflake id) {
    auto window = new GuildSettingsWindow(id);
    ManageHeapWindow(window);
    window->show();
}

void Abaddon::ActionAddRecipient(Snowflake channel_id) {
    FriendPickerDialog dlg(*m_main_window);
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK) {
        auto user_id = dlg.GetUserID();
        m_discord.AddGroupDMRecipient(channel_id, user_id);
    }
}

void Abaddon::ActionViewPins(Snowflake channel_id) {
    const auto data = m_discord.GetChannel(channel_id);
    if (!data.has_value()) return;
    auto window = new PinnedWindow(*data);
    ManageHeapWindow(window);
    window->show();
}

void Abaddon::ActionViewThreads(Snowflake channel_id) {
    auto data = m_discord.GetChannel(channel_id);
    if (!data.has_value()) return;
    if (data->IsThread()) {
        data = m_discord.GetChannel(*data->ParentID);
        if (!data.has_value()) return;
    }
    auto window = new ThreadsWindow(*data);
    ManageHeapWindow(window);
    window->show();
}

bool Abaddon::ShowConfirm(const Glib::ustring &prompt, Gtk::Window *window) {
    ConfirmDialog dlg(window != nullptr ? *window : *m_main_window);
    dlg.SetConfirmText(prompt);
    return dlg.run() == Gtk::RESPONSE_OK;
}

void Abaddon::ActionReloadCSS() {
    try {
        Gtk::StyleContext::remove_provider_for_screen(Gdk::Screen::get_default(), m_css_provider);
        m_css_provider->load_from_path(GetCSSPath("/" + m_settings.GetMainCSS()));
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), m_css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        Gtk::StyleContext::remove_provider_for_screen(Gdk::Screen::get_default(), m_css_low_provider);
        m_css_low_provider->load_from_path(GetCSSPath("/application-low-priority.css"));
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), m_css_low_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION - 1);
    } catch (Glib::Error &e) {
        Gtk::MessageDialog dlg(*m_main_window, "css failed to load (" + e.what() + ")", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }
}

ImageManager &Abaddon::GetImageManager() {
    return m_img_mgr;
}

EmojiResource &Abaddon::GetEmojis() {
    return m_emojis;
}

int main(int argc, char **argv) {
    if (std::getenv("ABADDON_NO_FC") == nullptr)
        Platform::SetupFonts();
#if defined(_WIN32) && defined(_MSC_VER)
    TCHAR buf[2] { 0 };
    GetEnvironmentVariableA("GTK_CSD", buf, sizeof(buf));
    if (buf[0] != '1')
        SetEnvironmentVariableA("GTK_CSD", "0");
#endif
    Gtk::Main::init_gtkmm_internals(); // why???
    return Abaddon::Get().StartGTK();
}
