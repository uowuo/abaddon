#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <gtkmm/application.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/statusicon.h>
#include "discord/discord.hpp"
#include "windows/mainwindow.hpp"
#include "settings.hpp"
#include "imgmanager.hpp"
#include "emojis.hpp"
#include "notifications/notifications.hpp"
#include "audio/manager.hpp"

#define APP_TITLE "Abaddon"

class AudioManager;

class Abaddon {
private:
    Abaddon();

public:
    static Abaddon &Get();

    Abaddon(const Abaddon &) = delete;
    Abaddon &operator=(const Abaddon &) = delete;
    Abaddon(Abaddon &&) = delete;
    Abaddon &operator=(Abaddon &&) = delete;

    int StartGTK();
    void OnShutdown();

    void StartDiscord();
    void StopDiscord();

    void LoadFromSettings();

    void ActionConnect();
    void ActionDisconnect();
    void ActionSetToken();
    void ActionLoginQR();
    void ActionJoinGuildDialog();
    void ActionChannelOpened(Snowflake id, bool expand_to = true);
    void ActionChatInputSubmit(ChatSubmitParams data);
    void ActionChatLoadHistory(Snowflake id);
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
    void ActionViewPins(Snowflake channel_id);
    void ActionViewThreads(Snowflake channel_id);

#ifdef WITH_VOICE
    void ActionJoinVoiceChannel(Snowflake channel_id);
    void ActionDisconnectVoice();
#endif

    std::optional<Glib::ustring> ShowTextPrompt(const Glib::ustring &prompt, const Glib::ustring &title, const Glib::ustring &placeholder = "", Gtk::Window *window = nullptr);
    bool ShowConfirm(const Glib::ustring &prompt, Gtk::Window *window = nullptr);

    void ActionReloadCSS();

    ImageManager &GetImageManager();
    EmojiResource &GetEmojis();

#ifdef WITH_VOICE
    AudioManager &GetAudio();
#endif

    std::string GetDiscordToken() const;
    bool IsDiscordActive() const;

    DiscordClient &GetDiscordClient();
    const DiscordClient &GetDiscordClient() const;
    void DiscordOnReady();
    void DiscordOnMessageCreate(const Message &message);
    void DiscordOnMessageDelete(Snowflake id, Snowflake channel_id);
    void DiscordOnMessageUpdate(Snowflake id, Snowflake channel_id);
    void DiscordOnGuildMemberListUpdate(Snowflake guild_id);
    void DiscordOnThreadMemberListUpdate(const ThreadMemberListUpdateData &data);
    void DiscordOnReactionAdd(Snowflake message_id, const Glib::ustring &param);
    void DiscordOnReactionRemove(Snowflake message_id, const Glib::ustring &param);
    void DiscordOnGuildJoinRequestCreate(const GuildJoinRequestCreateData &data);
    void DiscordOnMessageSent(const Message &data);
    void DiscordOnDisconnect(bool is_reconnecting, GatewayCloseCode close_code);
    void DiscordOnThreadUpdate(const ThreadUpdateData &data);

#ifdef WITH_VOICE
    void OnVoiceConnected();
    void OnVoiceDisconnected();

    void ShowVoiceWindow();
#endif

    SettingsManager::Settings &GetSettings();

    Glib::RefPtr<Gtk::CssProvider> GetStyleProvider();

    void ShowUserMenu(const GdkEvent *event, Snowflake id, Snowflake guild_id);

    void ManageHeapWindow(Gtk::Window *window);

    static std::string GetCSSPath();
    static std::string GetResPath();
    static std::string GetStateCachePath();
    static std::string GetCSSPath(const std::string &path);
    static std::string GetResPath(const std::string &path);
    static std::string GetStateCachePath(const std::string &path);

    [[nodiscard]] Glib::RefPtr<Gtk::Application> GetApp();
    [[nodiscard]] bool IsMainWindowActive();
    [[nodiscard]] Snowflake GetActiveChannelID() const noexcept;

protected:
    void RunFirstTimeDiscordStartup();

    void ShowGuildVerificationGateDialog(Snowflake guild_id);

    void CheckMessagesForMembers(const ChannelData &chan, const std::vector<Message> &msgs);

    void SetupUserMenu();
    void SaveState();
    void LoadState();

    void AttachCSSMonitor();
    Glib::RefPtr<Gio::FileMonitor> m_main_css_monitor;

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
    Gtk::Menu *m_tray_menu;
    Gtk::MenuItem *m_tray_exit;

    void on_user_menu_insert_mention();
    void on_user_menu_ban();
    void on_user_menu_kick();
    void on_user_menu_copy_id();
    void on_user_menu_open_dm();
    void on_user_menu_remove_recipient();
    void on_tray_click();
    void on_tray_popup_menu(int button, int activate_time);
    void on_tray_menu_click();
    void on_window_hide();

private:
    SettingsManager m_settings;

    DiscordClient m_discord;
    std::string m_discord_token;

    std::unordered_set<Snowflake> m_channels_requested;
    std::unordered_set<Snowflake> m_channels_history_loaded;
    std::unordered_set<Snowflake> m_channels_history_loading;

    ImageManager m_img_mgr;
    EmojiResource m_emojis;

#ifdef WITH_VOICE
    AudioManager m_audio;
    Gtk::Window *m_voice_window = nullptr;
#endif

    mutable std::mutex m_mutex;
    Glib::RefPtr<Gtk::Application> m_gtk_app;
    Glib::RefPtr<Gtk::CssProvider> m_css_provider;
    Glib::RefPtr<Gtk::StatusIcon> m_tray;
    std::unique_ptr<MainWindow> m_main_window; // wah wah cant create a gtkstylecontext fuck you

    Notifications m_notifications;
};
