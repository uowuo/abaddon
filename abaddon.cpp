#include <gtkmm.h>
#include <memory>
#include <string>
#include "discord/discord.hpp"
#include "dialogs/token.hpp"
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

    m_main_window = std::make_unique<MainWindow>();
    m_main_window->SetAbaddon(this);
    m_main_window->set_title("Abaddon");
    m_main_window->show();
    m_main_window->UpdateMenuStatus();

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
    m_main_window->UpdateChannelListing();
}

void Abaddon::ActionConnect() {
    if (!m_discord.IsStarted())
        StartDiscord();
    m_main_window->UpdateMenuStatus();
}

void Abaddon::ActionDisconnect() {
    if (m_discord.IsStarted())
        StopDiscord();
    m_main_window->UpdateMenuStatus();
}

void Abaddon::ActionSetToken() {
    TokenDialog dlg(*m_main_window);
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK) {
        m_discord_token = dlg.GetToken();
        m_main_window->UpdateMenuStatus();
        m_settings.SetSetting("discord", "token", m_discord_token);
    }
}

int main(int argc, char **argv) {
    Abaddon abaddon;
    return abaddon.StartGTK();
}
