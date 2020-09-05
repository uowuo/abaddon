#include <gtkmm.h>
#include <memory>
#include <string>
#include <algorithm>
#include "discord/discord.hpp"
#include "dialogs/token.hpp"
#include "dialogs/editmessage.hpp"
#include "abaddon.hpp"

#ifdef _WIN32
    #pragma comment(lib, "crypt32.lib")
#endif

Abaddon::Abaddon()
    : m_settings("abaddon.ini") {
    m_discord.SetAbaddon(this);
    LoadFromSettings();
}

Abaddon::~Abaddon() {
    m_settings.Close();
    m_discord.Stop();
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
    m_main_window->SetAbaddon(this);
    m_main_window->set_title(APP_TITLE);
    m_main_window->show();
    m_main_window->UpdateComponents();

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
    std::string token = m_settings.GetSetting("discord", "token");
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

const DiscordClient &Abaddon::GetDiscordClient() const {
    std::scoped_lock<std::mutex> guard(m_mutex);
    return m_discord;
}

void Abaddon::DiscordNotifyReady() {
    m_main_window->UpdateComponents();
}

void Abaddon::DiscordNotifyChannelListFullRefresh() {
    m_main_window->UpdateChannelListing();
}

void Abaddon::DiscordNotifyMessageCreate(Snowflake id) {
    m_main_window->UpdateChatNewMessage(id);
}

void Abaddon::DiscordNotifyMessageDelete(Snowflake id, Snowflake channel_id) {
    m_main_window->UpdateChatMessageDeleted(id, channel_id);
}

void Abaddon::DiscordNotifyMessageUpdateContent(Snowflake id, Snowflake channel_id) {
    m_main_window->UpdateChatMessageEditContent(id, channel_id);
}

void Abaddon::DiscordNotifyGuildMemberListUpdate(Snowflake guild_id) {
    m_main_window->UpdateMembers();
}

void Abaddon::ActionConnect() {
    if (!m_discord.IsStarted())
        StartDiscord();
    m_main_window->UpdateComponents();
}

void Abaddon::ActionDisconnect() {
    if (m_discord.IsStarted())
        StopDiscord();
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

void Abaddon::ActionMoveGuildUp(Snowflake id) {
    auto order = m_discord.GetUserSortedGuilds();
    // get iter to target
    decltype(order)::iterator target_iter;
    for (auto it = order.begin(); it != order.end(); it++) {
        if (it->first == id) {
            target_iter = it;
            break;
        }
    }

    decltype(order)::iterator left = target_iter - 1;
    std::swap(*left, *target_iter);

    std::vector<Snowflake> new_sort;
    for (const auto &x : order)
        new_sort.push_back(x.first);

    m_discord.UpdateSettingsGuildPositions(new_sort);
}

void Abaddon::ActionMoveGuildDown(Snowflake id) {
    auto order = m_discord.GetUserSortedGuilds();
    // get iter to target
    decltype(order)::iterator target_iter;
    for (auto it = order.begin(); it != order.end(); it++) {
        if (it->first == id) {
            target_iter = it;
            break;
        }
    }

    decltype(order)::iterator right = target_iter + 1;
    std::swap(*right, *target_iter);

    std::vector<Snowflake> new_sort;
    for (const auto &x : order)
        new_sort.push_back(x.first);

    m_discord.UpdateSettingsGuildPositions(new_sort);
}

void Abaddon::ActionCopyGuildID(Snowflake id) {
    Gtk::Clipboard::get()->set_text(std::to_string(id));
}

void Abaddon::ActionListChannelItemClick(Snowflake id) {
    auto *channel = m_discord.GetChannel(id);
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
            if (msgs.size() > 0) {
                m_oldest_listed_message[id] = msgs.back();
                m_main_window->UpdateChatWindowContents();
            }

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

int main(int argc, char **argv) {
    Gtk::Main::init_gtkmm_internals(); // why???
    Abaddon abaddon;
    return abaddon.StartGTK();
}
