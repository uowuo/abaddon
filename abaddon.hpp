#include <gtkmm.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
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
    void ActionMoveGuildUp(Snowflake id);
    void ActionMoveGuildDown(Snowflake id);
    void ActionListChannelItemClick(Snowflake id);

    std::string GetDiscordToken() const;
    bool IsDiscordActive() const;

    const DiscordClient &GetDiscordClient() const;
    void DiscordNotifyReady();
    void DiscordNotifyChannelListFullRefresh();

private:
    DiscordClient m_discord;
    std::string m_discord_token;
    std::unordered_set<Snowflake> m_channels_requested;

    mutable std::mutex m_mutex;
    Glib::RefPtr<Gtk::Application> m_gtk_app;
    SettingsManager m_settings;
    std::unique_ptr<MainWindow> m_main_window; // wah wah cant create a gtkstylecontext fuck you
};
