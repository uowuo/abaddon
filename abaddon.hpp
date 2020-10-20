#include <gtkmm.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include "discord/discord.hpp"
#include "windows/mainwindow.hpp"
#include "settings.hpp"
#include "imgmanager.hpp"

#define APP_TITLE "Abaddon"

class Abaddon {
private:
    Abaddon();
    ~Abaddon();
    Abaddon(const Abaddon &) = delete;
    Abaddon &operator=(const Abaddon &) = delete;
    Abaddon(Abaddon &&) = delete;
    Abaddon &operator=(Abaddon &&) = delete;

public:
    static Abaddon &Get();

    int StartGTK();
    void StartDiscord();
    void StopDiscord();

    void LoadFromSettings();

    void ActionConnect();
    void ActionDisconnect();
    void ActionSetToken();
    void ActionJoinGuildDialog();
    void ActionMoveGuildUp(Snowflake id);
    void ActionMoveGuildDown(Snowflake id);
    void ActionListChannelItemClick(Snowflake id);
    void ActionChatInputSubmit(std::string msg, Snowflake channel);
    void ActionChatLoadHistory(Snowflake id);
    void ActionChatDeleteMessage(Snowflake channel_id, Snowflake id);
    void ActionChatEditMessage(Snowflake channel_id, Snowflake id);
    void ActionInsertMention(Snowflake id);
    void ActionLeaveGuild(Snowflake id);
    void ActionKickMember(Snowflake user_id, Snowflake guild_id);
    void ActionBanMember(Snowflake user_id, Snowflake guild_id);

    void ActionReloadCSS();

    ImageManager &GetImageManager();

    std::string GetDiscordToken() const;
    bool IsDiscordActive() const;

    DiscordClient &GetDiscordClient();
    const DiscordClient &GetDiscordClient() const;
    void DiscordOnReady();
    void DiscordOnChannelListRefresh();
    void DiscordOnMessageCreate(Snowflake id);
    void DiscordOnMessageDelete(Snowflake id, Snowflake channel_id);
    void DiscordOnMessageUpdate(Snowflake id, Snowflake channel_id);
    void DiscordOnGuildMemberListUpdate(Snowflake guild_id);
    void DiscordOnGuildCreate(Snowflake guild_id);
    void DiscordOnGuildDelete(Snowflake guild_id);
    void DiscordOnChannelDelete(Snowflake channel_id);
    void DiscordOnChannelUpdate(Snowflake channel_id);
    void DiscordOnChannelCreate(Snowflake channel_id);

    const SettingsManager &GetSettings() const;

private:
    DiscordClient m_discord;
    std::string m_discord_token;
    // todo make these map snowflake to attribs
    std::unordered_set<Snowflake> m_channels_requested;
    std::unordered_set<Snowflake> m_channels_history_loaded;
    std::unordered_map<Snowflake, Snowflake> m_oldest_listed_message;
    std::unordered_set<Snowflake> m_channels_history_loading;

    ImageManager m_img_mgr;

    mutable std::mutex m_mutex;
    Glib::RefPtr<Gtk::Application> m_gtk_app;
    Glib::RefPtr<Gtk::CssProvider> m_css_provider;
    SettingsManager m_settings;
    std::unique_ptr<MainWindow> m_main_window; // wah wah cant create a gtkstylecontext fuck you
};
