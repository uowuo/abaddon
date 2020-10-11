#include <gtkmm.h>
#include <memory>
#include <string>
#include <algorithm>
#include "discord/discord.hpp"
#include "dialogs/token.hpp"
#include "dialogs/editmessage.hpp"
#include "dialogs/joinguild.hpp"
#include "abaddon.hpp"

#ifdef _WIN32
    #pragma comment(lib, "crypt32.lib")
#endif

Abaddon::Abaddon()
    : m_settings("abaddon.ini") {
    LoadFromSettings();

    m_discord.signal_gateway_ready().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnReady));
    m_discord.signal_channel_list_refresh().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnChannelListRefresh));
    m_discord.signal_message_create().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageCreate));
    m_discord.signal_message_delete().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageDelete));
    m_discord.signal_message_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageUpdate));
    m_discord.signal_guild_member_list_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildMemberListUpdate));
    m_discord.signal_guild_create().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildCreate));
    m_discord.signal_guild_delete().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildDelete));
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
    m_main_window->show();
    m_main_window->UpdateComponents();

    m_main_window->signal_action_connect().connect(sigc::mem_fun(*this, &Abaddon::ActionConnect));
    m_main_window->signal_action_disconnect().connect(sigc::mem_fun(*this, &Abaddon::ActionDisconnect));
    m_main_window->signal_action_set_token().connect(sigc::mem_fun(*this, &Abaddon::ActionSetToken));
    m_main_window->signal_action_reload_css().connect(sigc::mem_fun(*this, &Abaddon::ActionReloadCSS));
    m_main_window->signal_action_join_guild().connect(sigc::mem_fun(*this, &Abaddon::ActionJoinGuildDialog));

    m_main_window->GetChannelList()->signal_action_channel_item_select().connect(sigc::mem_fun(*this, &Abaddon::ActionListChannelItemClick));
    m_main_window->GetChannelList()->signal_action_guild_move_up().connect(sigc::mem_fun(*this, &Abaddon::ActionMoveGuildUp));
    m_main_window->GetChannelList()->signal_action_guild_move_down().connect(sigc::mem_fun(*this, &Abaddon::ActionMoveGuildDown));
    m_main_window->GetChannelList()->signal_action_guild_copy_id().connect(sigc::mem_fun(*this, &Abaddon::ActionCopyGuildID));
    m_main_window->GetChannelList()->signal_action_guild_leave().connect(sigc::mem_fun(*this, &Abaddon::ActionLeaveGuild));

    m_main_window->GetChatWindow()->signal_action_message_delete().connect(sigc::mem_fun(*this, &Abaddon::ActionChatDeleteMessage));
    m_main_window->GetChatWindow()->signal_action_message_edit().connect(sigc::mem_fun(*this, &Abaddon::ActionChatEditMessage));
    m_main_window->GetChatWindow()->signal_action_chat_submit().connect(sigc::mem_fun(*this, &Abaddon::ActionChatInputSubmit));
    m_main_window->GetChatWindow()->signal_action_chat_load_history().connect(sigc::mem_fun(*this, &Abaddon::ActionChatLoadHistory));
    m_main_window->GetChatWindow()->signal_action_channel_click().connect(sigc::mem_fun(*this, &Abaddon::ActionListChannelItemClick)); // rename me

    m_main_window->GetMemberList()->signal_action_insert_mention().connect(sigc::mem_fun(*this, &Abaddon::ActionInsertMention));

    ActionReloadCSS();

    m_gtk_app->signal_shutdown().connect([&]() {
        StopDiscord();
    });

    if (!m_settings.IsValid()) {
        Gtk::MessageDialog dlg(*m_main_window, "The settings file could not be created!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.run();
    }

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

void Abaddon::DiscordOnChannelListRefresh() {
    m_main_window->UpdateChannelListing();
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
    m_main_window->UpdateChannelListing();
}

void Abaddon::DiscordOnGuildDelete(Snowflake guild_id) {
    m_main_window->UpdateChannelListing();
}

const SettingsManager &Abaddon::GetSettings() const {
    return m_settings;
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

void Abaddon::ActionMoveGuildUp(Snowflake id) {
    auto order = m_discord.GetUserSortedGuilds();
    // get iter to target
    decltype(order)::iterator target_iter;
    for (auto it = order.begin(); it != order.end(); it++) {
        if (*it == id) {
            target_iter = it;
            break;
        }
    }

    decltype(order)::iterator left = target_iter - 1;
    std::swap(*left, *target_iter);

    m_discord.UpdateSettingsGuildPositions(order);
}

void Abaddon::ActionMoveGuildDown(Snowflake id) {
    auto order = m_discord.GetUserSortedGuilds();
    // get iter to target
    decltype(order)::iterator target_iter;
    for (auto it = order.begin(); it != order.end(); it++) {
        if (*it == id) {
            target_iter = it;
            break;
        }
    }

    decltype(order)::iterator right = target_iter + 1;
    std::swap(*right, *target_iter);

    m_discord.UpdateSettingsGuildPositions(order);
}

void Abaddon::ActionCopyGuildID(Snowflake id) {
    Gtk::Clipboard::get()->set_text(std::to_string(id));
}

void Abaddon::ActionListChannelItemClick(Snowflake id) {
    if (id == m_main_window->GetChatActiveChannel()) return;

    auto *channel = m_discord.GetChannel(id);
    if (channel->Type != ChannelType::DM && channel->Type != ChannelType::GROUP_DM)
        m_discord.SendLazyLoad(id);

    if (channel->Type == ChannelType::GUILD_TEXT)
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
    EditMessageDialog dlg(*m_main_window);
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
    m_discord.LeaveGuild(id);
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

int main(int argc, char **argv) {
    Gtk::Main::init_gtkmm_internals(); // why???
    return Abaddon::Get().StartGTK();
}
