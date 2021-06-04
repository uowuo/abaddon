#include <gtkmm.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include "discord/discord.hpp"
#include "windows/mainwindow.hpp"
#include "settings.hpp"
#include "imgmanager.hpp"
#include "emojis.hpp"

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
    void ActionChannelOpened(Snowflake id);
    void ActionChatInputSubmit(std::string msg, Snowflake channel, Snowflake referenced_message);
    void ActionChatLoadHistory(Snowflake id);
    void ActionChatDeleteMessage(Snowflake channel_id, Snowflake id);
    void ActionChatEditMessage(Snowflake channel_id, Snowflake id);
    void ActionInsertMention(Snowflake id);
    void ActionLeaveGuild(Snowflake id);
    void ActionKickMember(Snowflake user_id, Snowflake guild_id);
    void ActionBanMember(Snowflake user_id, Snowflake guild_id);
    void ActionSetStatus();
    void ActionReactionAdd(Snowflake id, const Glib::ustring &param);
    void ActionReactionRemove(Snowflake id, const Glib::ustring &param);
    void ActionGuildSettings(Snowflake id);
    void ActionAddRecipient(Snowflake channel_id);

    bool ShowConfirm(const Glib::ustring &prompt, Gtk::Window *window = nullptr);

    void ActionReloadSettings();
    void ActionReloadCSS();

    ImageManager &GetImageManager();
    EmojiResource &GetEmojis();

    std::string GetDiscordToken() const;
    bool IsDiscordActive() const;

    DiscordClient &GetDiscordClient();
    const DiscordClient &GetDiscordClient() const;
    void DiscordOnReady();
    void DiscordOnMessageCreate(const Message &message);
    void DiscordOnMessageDelete(Snowflake id, Snowflake channel_id);
    void DiscordOnMessageUpdate(Snowflake id, Snowflake channel_id);
    void DiscordOnGuildMemberListUpdate(Snowflake guild_id);
    void DiscordOnGuildCreate(const GuildData &guild);
    void DiscordOnGuildDelete(Snowflake guild_id);
    void DiscordOnChannelDelete(Snowflake channel_id);
    void DiscordOnChannelUpdate(Snowflake channel_id);
    void DiscordOnChannelCreate(Snowflake channel_id);
    void DiscordOnGuildUpdate(Snowflake guild_id);
    void DiscordOnReactionAdd(Snowflake message_id, const Glib::ustring &param);
    void DiscordOnReactionRemove(Snowflake message_id, const Glib::ustring &param);
    void DiscordOnGuildJoinRequestCreate(const GuildJoinRequestCreateData &data);
    void DiscordOnMessageSent(const Message &data);
    void DiscordOnDisconnect(bool is_reconnecting, GatewayCloseCode close_code);

    const SettingsManager &GetSettings() const;

    Glib::RefPtr<Gtk::CssProvider> GetStyleProvider();

    void ShowUserMenu(const GdkEvent *event, Snowflake id, Snowflake guild_id);

    void ManageHeapWindow(Gtk::Window *window);

protected:
    void ShowGuildVerificationGateDialog(Snowflake guild_id);

    void SetupUserMenu();

    Snowflake m_shown_user_menu_id;
    Snowflake m_shown_user_menu_guild_id;

    Gtk::Menu *m_user_menu;
    Gtk::MenuItem *m_user_menu_info;
    Gtk::MenuItem *m_user_menu_insert_mention;
    Gtk::MenuItem *m_user_menu_ban;
    Gtk::MenuItem *m_user_menu_kick;
    Gtk::MenuItem *m_user_menu_copy_id;
    Gtk::MenuItem *m_user_menu_open_dm;
    Gtk::MenuItem *m_user_menu_roles;
    Gtk::MenuItem *m_user_menu_remove_recipient;
    Gtk::Menu *m_user_menu_roles_submenu;

    void on_user_menu_insert_mention();
    void on_user_menu_ban();
    void on_user_menu_kick();
    void on_user_menu_copy_id();
    void on_user_menu_open_dm();
    void on_user_menu_remove_recipient();

private:
    SettingsManager m_settings;

    DiscordClient m_discord;
    std::string m_discord_token;

    std::unordered_set<Snowflake> m_channels_requested;
    std::unordered_set<Snowflake> m_channels_history_loaded;
    std::unordered_set<Snowflake> m_channels_history_loading;

    ImageManager m_img_mgr;
    EmojiResource m_emojis;

    mutable std::mutex m_mutex;
    Glib::RefPtr<Gtk::Application> m_gtk_app;
    Glib::RefPtr<Gtk::CssProvider> m_css_provider;
    Glib::RefPtr<Gtk::CssProvider> m_css_low_provider; // registered with a lower priority to allow better customization
    std::unique_ptr<MainWindow> m_main_window;         // wah wah cant create a gtkstylecontext fuck you
};
