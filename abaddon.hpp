#include <gtkmm.h>
#include <memory>
#include <mutex>
#include <string>
#include "discord/discord.hpp"
#include "windows/mainwindow.hpp"
#include "settings.hpp"

class Abaddon {
public:
    Abaddon();
    ~Abaddon();

    int StartGTK();
    void StartDiscord();
    void StopDiscord();

    void LoadFromSettings();

    void ActionConnect();
    void ActionDisconnect();
    void ActionSetToken();

    std::string GetDiscordToken() const;
    bool IsDiscordActive() const;

    const DiscordClient &GetDiscordClient() const;
    void DiscordNotifyReady();

private:
    std::string m_discord_token;
    mutable std::mutex m_mutex;
    Glib::RefPtr<Gtk::Application> m_gtk_app;
    DiscordClient m_discord;
    SettingsManager m_settings;
    std::unique_ptr<MainWindow> m_main_window; // wah wah cant create a gtkstylecontext fuck you
};
