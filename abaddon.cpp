#include <gtkmm.h>
#include <memory>
#include <string>
#include <algorithm>
#include "discord/discord.hpp"
#include "dialogs/token.hpp"
#include "dialogs/editmessage.hpp"
#include "dialogs/joinguild.hpp"
#include "dialogs/confirm.hpp"
#include "dialogs/setstatus.hpp"
#include "abaddon.hpp"

#ifdef _WIN32
    #pragma comment(lib, "crypt32.lib")
#endif

Abaddon::Abaddon()
    : m_settings("abaddon.ini")
    , m_emojis("res/emojis.bin")
    , m_discord(m_settings.GetSettingBool("discord", "memory_db", false)) { // stupid but easy
    LoadFromSettings();

    m_discord.signal_gateway_ready().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnReady));
    m_discord.signal_message_create().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageCreate));
    m_discord.signal_message_delete().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageDelete));
    m_discord.signal_message_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageUpdate));
    m_discord.signal_guild_member_list_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildMemberListUpdate));
    m_discord.signal_guild_create().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildCreate));
    m_discord.signal_guild_delete().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildDelete));
    m_discord.signal_channel_delete().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnChannelDelete));
    m_discord.signal_channel_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnChannelUpdate));
    m_discord.signal_channel_create().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnChannelCreate));
    m_discord.signal_guild_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildUpdate));
}

Abaddon::~Abaddon() {
    m_settings.Close();
    m_discord.Stop();
}

Abaddon &Abaddon::Get() {
    static Abaddon instance;
    return instance;
}

int Abaddon::StartGTK() {
    m_gtk_app = Gtk::Application::create("com.github.lorpus.abaddon");

    // tmp css stuff
    m_css_provider = Gtk::CssProvider::create();
    m_css_provider->signal_parsing_error().connect([this](const Glib::RefPtr<const Gtk::CssSection> &section, const Glib::Error &error) {
        Gtk::MessageDialog dlg(*m_main_window, "main.css failed parsing (" + error.what() + ")", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.run();
    });

    m_main_window = std::make_unique<MainWindow>();
    m_main_window->set_title(APP_TITLE);
    m_main_window->UpdateComponents();

    // crashes for some stupid reason if i put it somewhere else
    m_user_menu = Gtk::manage(new Gtk::Menu);
    m_user_menu_insert_mention = Gtk::manage(new Gtk::MenuItem("Insert Mention"));
    m_user_menu_ban = Gtk::manage(new Gtk::MenuItem("Ban"));
    m_user_menu_kick = Gtk::manage(new Gtk::MenuItem("Kick"));
    m_user_menu_copy_id = Gtk::manage(new Gtk::MenuItem("Copy ID"));
    m_user_menu_open_dm = Gtk::manage(new Gtk::MenuItem("Open DM"));
    m_user_menu_insert_mention->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_insert_mention));
    m_user_menu_ban->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_ban));
    m_user_menu_kick->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_kick));
    m_user_menu_copy_id->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_copy_id));
    m_user_menu_open_dm->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_open_dm));
    m_user_menu->append(*m_user_menu_insert_mention);
    m_user_menu->append(*m_user_menu_ban);
    m_user_menu->append(*m_user_menu_kick);
    m_user_menu->append(*m_user_menu_open_dm);
    m_user_menu->append(*m_user_menu_copy_id);
    m_user_menu->show_all();

    m_main_window->signal_action_connect().connect(sigc::mem_fun(*this, &Abaddon::ActionConnect));
    m_main_window->signal_action_disconnect().connect(sigc::mem_fun(*this, &Abaddon::ActionDisconnect));
    m_main_window->signal_action_set_token().connect(sigc::mem_fun(*this, &Abaddon::ActionSetToken));
    m_main_window->signal_action_reload_css().connect(sigc::mem_fun(*this, &Abaddon::ActionReloadCSS));
    m_main_window->signal_action_join_guild().connect(sigc::mem_fun(*this, &Abaddon::ActionJoinGuildDialog));
    m_main_window->signal_action_set_status().connect(sigc::mem_fun(*this, &Abaddon::ActionSetStatus));

    m_main_window->signal_action_show_user_menu().connect(sigc::mem_fun(*this, &Abaddon::ShowUserMenu));

    m_main_window->GetChannelList()->signal_action_channel_item_select().connect(sigc::mem_fun(*this, &Abaddon::ActionChannelOpened));
    m_main_window->GetChannelList()->signal_action_guild_leave().connect(sigc::mem_fun(*this, &Abaddon::ActionLeaveGuild));

    m_main_window->GetChatWindow()->signal_action_message_delete().connect(sigc::mem_fun(*this, &Abaddon::ActionChatDeleteMessage));
    m_main_window->GetChatWindow()->signal_action_message_edit().connect(sigc::mem_fun(*this, &Abaddon::ActionChatEditMessage));
    m_main_window->GetChatWindow()->signal_action_chat_submit().connect(sigc::mem_fun(*this, &Abaddon::ActionChatInputSubmit));
    m_main_window->GetChatWindow()->signal_action_chat_load_history().connect(sigc::mem_fun(*this, &Abaddon::ActionChatLoadHistory));
    m_main_window->GetChatWindow()->signal_action_channel_click().connect(sigc::mem_fun(*this, &Abaddon::ActionChannelOpened));
    m_main_window->GetChatWindow()->signal_action_insert_mention().connect(sigc::mem_fun(*this, &Abaddon::ActionInsertMention));

    ActionReloadCSS();

    m_gtk_app->signal_shutdown().connect([&]() {
        StopDiscord();
    });

    if (!m_settings.IsValid()) {
        Gtk::MessageDialog dlg(*m_main_window, "The settings file could not be created!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.run();
    }

    if (!m_emojis.Load()) {
        Gtk::MessageDialog dlg(*m_main_window, "The emoji file couldn't be loaded!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.run();
    }

    if (!m_discord.IsStoreValid()) {
        Gtk::MessageDialog dlg(*m_main_window, "The Discord cache could not be created!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.run();
        return 1;
    }

    m_main_window->show();
    return m_gtk_app->run(*m_main_window);
}

void Abaddon::LoadFromSettings() {
    std::string token = m_settings.GetSettingString("discord", "token");
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
}

void Abaddon::DiscordOnMessageCreate(Snowflake id) {
    m_main_window->UpdateChatNewMessage(id);
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

void Abaddon::DiscordOnGuildCreate(Snowflake guild_id) {
    m_main_window->UpdateChannelsNewGuild(guild_id);
}

void Abaddon::DiscordOnGuildDelete(Snowflake guild_id) {
    m_main_window->UpdateChannelsRemoveGuild(guild_id);
}

void Abaddon::DiscordOnChannelDelete(Snowflake channel_id) {
    m_main_window->UpdateChannelsRemoveChannel(channel_id);
}

void Abaddon::DiscordOnChannelUpdate(Snowflake channel_id) {
    m_main_window->UpdateChannelsUpdateChannel(channel_id);
}

void Abaddon::DiscordOnChannelCreate(Snowflake channel_id) {
    m_main_window->UpdateChannelsCreateChannel(channel_id);
}

void Abaddon::DiscordOnGuildUpdate(Snowflake guild_id) {
    m_main_window->UpdateChannelsUpdateGuild(guild_id);
}

const SettingsManager &Abaddon::GetSettings() const {
    return m_settings;
}

void Abaddon::ShowUserMenu(const GdkEvent *event, Snowflake id, Snowflake guild_id) {
    m_shown_user_menu_id = id;
    m_shown_user_menu_guild_id = guild_id;

    const auto me = m_discord.GetUserData().ID;
    if (me == id) {
        m_user_menu_ban->set_sensitive(false);
        m_user_menu_kick->set_sensitive(false);
        m_user_menu_open_dm->set_sensitive(false);
    } else {
        const bool has_kick = m_discord.HasGuildPermission(me, guild_id, Permission::KICK_MEMBERS);
        const bool has_ban = m_discord.HasGuildPermission(me, guild_id, Permission::BAN_MEMBERS);
        const bool can_manage = m_discord.CanManageMember(guild_id, me, id);

        m_user_menu_kick->set_sensitive(has_kick && can_manage);
        m_user_menu_ban->set_sensitive(has_ban && can_manage);
        m_user_menu_open_dm->set_sensitive(true);
    }

    m_user_menu->popup_at_pointer(event);
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
        m_discord.CreateDM(m_shown_user_menu_id);
}

void Abaddon::ActionConnect() {
    if (!m_discord.IsStarted())
        StartDiscord();
    m_main_window->UpdateComponents();
}

void Abaddon::ActionDisconnect() {
    if (m_discord.IsStarted())
        StopDiscord();
    m_channels_history_loaded.clear();
    m_channels_history_loading.clear();
    m_channels_requested.clear();
    m_oldest_listed_message.clear();
    m_main_window->set_title(APP_TITLE);
    m_main_window->UpdateComponents();
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
    if (id == m_main_window->GetChatActiveChannel()) return;

    auto *channel = m_discord.GetChannel(id);
    if (channel->Type != ChannelType::DM && channel->Type != ChannelType::GROUP_DM)
        m_discord.SendLazyLoad(id);

    if (channel->Type == ChannelType::GUILD_TEXT || channel->Type == ChannelType::GUILD_NEWS)
        m_main_window->set_title(std::string(APP_TITLE) + " - #" + channel->Name);
    else {
        std::string display;
        if (channel->Recipients.size() > 1)
            display = std::to_string(channel->Recipients.size()) + " users";
        else
            display = channel->Recipients[0].Username;
        m_main_window->set_title(std::string(APP_TITLE) + " - " + display);
    }
    m_main_window->UpdateChatActiveChannel(id);
    if (m_channels_requested.find(id) == m_channels_requested.end()) {
        m_discord.FetchMessagesInChannel(id, [this, id](const std::vector<Snowflake> &msgs) {
            if (msgs.size() > 0)
                m_oldest_listed_message[id] = msgs.back();

            m_main_window->UpdateChatWindowContents();
            m_channels_requested.insert(id);
        });
    } else {
        m_main_window->UpdateChatWindowContents();
    }
}

void Abaddon::ActionChatLoadHistory(Snowflake id) {
    if (m_channels_history_loaded.find(id) != m_channels_history_loaded.end())
        return;

    if (m_channels_history_loading.find(id) != m_channels_history_loading.end())
        return;

    Snowflake before_id = m_main_window->GetChatOldestListedMessage();
    auto knownset = m_discord.GetMessagesForChannel(id);
    std::vector<Snowflake> knownvec(knownset.begin(), knownset.end());
    std::sort(knownvec.begin(), knownvec.end());
    auto latest = std::find_if(knownvec.begin(), knownvec.end(), [&before_id](Snowflake x) -> bool { return x == before_id; });
    int distance = std::distance(knownvec.begin(), latest);

    if (distance >= 50) {
        m_main_window->UpdateChatPrependHistory(std::vector<Snowflake>(knownvec.begin() + distance - 50, knownvec.begin() + distance));
        return;
    }

    m_channels_history_loading.insert(id);

    m_discord.FetchMessagesInChannelBefore(id, m_oldest_listed_message[id], [this, id](const std::vector<Snowflake> &msgs) {
        m_channels_history_loading.erase(id);

        if (msgs.size() == 0) {
            m_channels_history_loaded.insert(id);
        } else {
            m_oldest_listed_message[id] = msgs.back();
            m_main_window->UpdateChatPrependHistory(msgs);
        }
    });
}

void Abaddon::ActionChatInputSubmit(std::string msg, Snowflake channel) {
    if (msg.substr(0, 7) == "/shrug " || msg == "/shrug")
        msg = msg.substr(6) + "\xC2\xAF\x5C\x5F\x28\xE3\x83\x84\x29\x5F\x2F\xC2\xAF"; // this is important
    m_discord.SendChatMessage(msg, channel);
}

void Abaddon::ActionChatDeleteMessage(Snowflake channel_id, Snowflake id) {
    m_discord.DeleteMessage(channel_id, id);
}

void Abaddon::ActionChatEditMessage(Snowflake channel_id, Snowflake id) {
    const auto msg = m_discord.GetMessage(id);
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
    const auto *guild = m_discord.GetGuild(id);
    if (guild != nullptr)
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
    Activity activity;
    activity.Name = activity_name;
    activity.Type = activity_type;
    m_discord.UpdateStatus(status, false, activity);
}

void Abaddon::ActionReloadCSS() {
    try {
        Gtk::StyleContext::remove_provider_for_screen(Gdk::Screen::get_default(), m_css_provider);
        m_css_provider->load_from_path("./css/main.css");
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), m_css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    } catch (Glib::Error &e) {
        Gtk::MessageDialog dlg(*m_main_window, "main.css failed to load (" + e.what() + ")", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
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
    Gtk::Main::init_gtkmm_internals(); // why???
    return Abaddon::Get().StartGTK();
}
