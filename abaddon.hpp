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
    void ActionCopyGuildID(Snowflake id);
    void ActionListChannelItemClick(Snowflake id);
    void ActionChatInputSubmit(std::string msg, Snowflake channel);
    void ActionChatLoadHistory(Snowflake id);
    void ActionChatDeleteMessage(Snowflake channel_id, Snowflake id);
    void ActionChatEditMessage(Snowflake channel_id, Snowflake id);

    void ActionReloadCSS();

    std::string GetDiscordToken() const;
    bool IsDiscordActive() const;

    const DiscordClient &GetDiscordClient() const;
    void DiscordNotifyReady();
    void DiscordNotifyChannelListFullRefresh();
    void DiscordNotifyMessageCreate(Snowflake id);
    void DiscordNotifyMessageDelete(Snowflake id, Snowflake channel_id);
    void DiscordNotifyMessageUpdateContent(Snowflake id, Snowflake channel_id);

private:
    DiscordClient m_discord;
    std::string m_discord_token;
    // todo make these map snowflake to attribs
    std::unordered_set<Snowflake> m_channels_requested;
    std::unordered_set<Snowflake> m_channels_history_loaded;
    std::unordered_map<Snowflake, Snowflake> m_oldest_listed_message;
    std::unordered_set<Snowflake> m_channels_history_loading;

    mutable std::mutex m_mutex;
    Glib::RefPtr<Gtk::Application> m_gtk_app;
    Glib::RefPtr<Gtk::CssProvider> m_css_provider;
    SettingsManager m_settings;
    std::unique_ptr<MainWindow> m_main_window; // wah wah cant create a gtkstylecontext fuck you
};
