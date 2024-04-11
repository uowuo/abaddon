#include "abaddon.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <algorithm>
#include <gtkmm.h>
#include "platform.hpp"
#include "audio/manager.hpp"
#include "discord/discord.hpp"
#include "dialogs/token.hpp"
#include "dialogs/confirm.hpp"
#include "dialogs/setstatus.hpp"
#include "dialogs/friendpicker.hpp"
#include "dialogs/verificationgate.hpp"
#include "dialogs/textinput.hpp"
#include "windows/guildsettingswindow.hpp"
#include "windows/profilewindow.hpp"
#include "windows/pinnedwindow.hpp"
#include "windows/threadswindow.hpp"
#include "windows/voicewindow.hpp"
#include "startup.hpp"
#include "notifications/notifications.hpp"
#include "remoteauth/remoteauthdialog.hpp"
#include "util.hpp"

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>

void macOSThemeChanged() {
    CFPropertyListRef appearanceName = CFPreferencesCopyAppValue(CFSTR("AppleInterfaceStyle"), kCFPreferencesAnyApplication);
    if (appearanceName != NULL && CFGetTypeID(appearanceName) == CFStringGetTypeID() && CFStringCompare((CFStringRef)appearanceName, CFSTR("Dark"), 0) == kCFCompareEqualTo) {
        Gtk::Settings::get_default()->set_property("gtk-application-prefer-dark-theme", true);
    } else {
        Gtk::Settings::get_default()->set_property("gtk-application-prefer-dark-theme", false);
    }
}

void macOSThemeChangedCallback(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) {
    macOSThemeChanged();
}
#endif

#ifdef WITH_LIBHANDY
#include <handy.h>
#endif

#ifdef _WIN32
#pragma comment(lib, "crypt32.lib")
#endif

Abaddon::Abaddon()
    : m_settings(Platform::FindConfigFile())
    , m_discord(GetSettings().UseMemoryDB) // stupid but easy
    , m_emojis(GetResPath("/emojis.db"))
#ifdef WITH_VOICE
    , m_audio(GetSettings().Backends)
#endif
{
    LoadFromSettings();

    // todo: set user agent for non-client(?)
    std::string ua = GetSettings().UserAgent;
    m_discord.SetUserAgent(ua);

    // todo rename funcs
    m_discord.signal_gateway_ready_supplemental().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnReady));
    m_discord.signal_message_create().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageCreate));
    m_discord.signal_message_delete().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageDelete));
    m_discord.signal_message_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageUpdate));
    m_discord.signal_guild_member_list_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildMemberListUpdate));
    m_discord.signal_thread_member_list_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnThreadMemberListUpdate));
    m_discord.signal_reaction_add().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnReactionAdd));
    m_discord.signal_reaction_remove().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnReactionRemove));
    m_discord.signal_guild_join_request_create().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnGuildJoinRequestCreate));
    m_discord.signal_thread_update().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnThreadUpdate));
    m_discord.signal_message_sent().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnMessageSent));
    m_discord.signal_disconnected().connect(sigc::mem_fun(*this, &Abaddon::DiscordOnDisconnect));

#ifdef WITH_VOICE
    m_discord.signal_voice_connected().connect(sigc::mem_fun(*this, &Abaddon::OnVoiceConnected));
    m_discord.signal_voice_disconnected().connect(sigc::mem_fun(*this, &Abaddon::OnVoiceDisconnected));
    m_discord.signal_voice_speaking().connect([this](const VoiceSpeakingData &m) {
        spdlog::get("voice")->debug("{} SSRC: {}", m.UserID, m.SSRC);
        m_audio.AddSSRC(m.SSRC);
    });
#endif

    m_discord.signal_channel_accessibility_changed().connect([this](Snowflake id, bool accessible) {
        if (!accessible)
            m_channels_requested.erase(id);
    });
    if (GetSettings().Prefetch) {
        m_discord.signal_message_create().connect([this](const Message &message) {
            if (message.Author.HasAvatar())
                m_img_mgr.Prefetch(message.Author.GetAvatarURL());
            for (const auto &attachment : message.Attachments) {
                if (IsURLViewableImage(attachment.ProxyURL))
                    m_img_mgr.Prefetch(attachment.ProxyURL);
            }
        });
    }

#ifdef WITH_VOICE
    m_audio.SetVADMethod(GetSettings().VAD);
#endif
}

Abaddon &Abaddon::Get() {
    static Abaddon instance;
    return instance;
}

#ifdef _WIN32
constexpr static guint BUTTON_BACK = 4;
constexpr static guint BUTTON_FORWARD = 5;
#else
constexpr static guint BUTTON_BACK = 8;
constexpr static guint BUTTON_FORWARD = 9;
#endif

static bool HandleButtonEvents(GdkEvent *event, MainWindow *main_window) {
    if (event->type != GDK_BUTTON_PRESS) return false;

    auto *widget = gtk_get_event_widget(event);
    if (widget == nullptr) return false;
    auto *window = gtk_widget_get_toplevel(widget);
    if (static_cast<void *>(window) != static_cast<void *>(main_window->gobj())) return false; // is this the right way???

#ifdef WITH_LIBHANDY
    switch (event->button.button) {
        case BUTTON_BACK:
            main_window->GoBack();
            break;
        case BUTTON_FORWARD:
            main_window->GoForward();
            break;
    }
#endif

    return false;
}

static bool HandleKeyEvents(GdkEvent *event, MainWindow *main_window) {
    if (event->type != GDK_KEY_PRESS) return false;

    auto *widget = gtk_get_event_widget(event);
    if (widget == nullptr) return false;
    auto *window = gtk_widget_get_toplevel(widget);
    if (static_cast<void *>(window) != static_cast<void *>(main_window->gobj())) return false;

    const bool ctrl = (event->key.state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK;
    const bool shft = (event->key.state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK;

    constexpr static guint EXCLUDE_STATES = GDK_CONTROL_MASK | GDK_SHIFT_MASK;

    if (!(event->key.state & EXCLUDE_STATES) && event->key.keyval == GDK_KEY_Alt_L) {
        if (Abaddon::Get().GetSettings().AltMenu) {
            main_window->ToggleMenuVisibility();
        }
    }

#ifdef WITH_LIBHANDY
    if (ctrl) {
        switch (event->key.keyval) {
            case GDK_KEY_Tab:
            case GDK_KEY_KP_Tab:
            case GDK_KEY_ISO_Left_Tab:
                if (shft)
                    main_window->GoToPreviousTab();
                else
                    main_window->GoToNextTab();
                return true;
            case GDK_KEY_1:
            case GDK_KEY_2:
            case GDK_KEY_3:
            case GDK_KEY_4:
            case GDK_KEY_5:
            case GDK_KEY_6:
            case GDK_KEY_7:
            case GDK_KEY_8:
            case GDK_KEY_9:
                main_window->GoToTab(event->key.keyval - GDK_KEY_1);
                return true;
            case GDK_KEY_0:
                main_window->GoToTab(9);
                return true;
        }
    }
#endif

    return false;
}

static void MainEventHandler(GdkEvent *event, void *main_window) {
    if (HandleButtonEvents(event, static_cast<MainWindow *>(main_window))) return;
    if (HandleKeyEvents(event, static_cast<MainWindow *>(main_window))) return;
    gtk_main_do_event(event);
}

int Abaddon::StartGTK() {
    m_gtk_app = Gtk::Application::create("com.github.uowuo.abaddon");
    Glib::set_application_name(APP_TITLE);

#ifdef WITH_LIBHANDY
    m_gtk_app->signal_activate().connect([] {
        hdy_init();
    });
#endif

    m_css_provider = Gtk::CssProvider::create();
    m_css_provider->signal_parsing_error().connect([](const Glib::RefPtr<const Gtk::CssSection> &section, const Glib::Error &error) {
        Gtk::MessageDialog dlg("css failed parsing (" + error.what() + ")", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    });

#ifdef _WIN32
    bool png_found = false;
    bool gif_found = false;
    for (const auto &fmt : Gdk::Pixbuf::get_formats()) {
        if (fmt.get_name() == "png")
            png_found = true;
        else if (fmt.get_name() == "gif")
            gif_found = true;
    }

    if (!png_found) {
        Gtk::MessageDialog dlg("The PNG pixbufloader wasn't detected. Abaddon may not work as a result.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }

    if (!gif_found) {
        Gtk::MessageDialog dlg("The GIF pixbufloader wasn't detected. Animations may not display as a result.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }
#endif

    m_main_window = std::make_unique<MainWindow>();
    m_main_window->set_title(APP_TITLE);
    m_main_window->set_position(Gtk::WIN_POS_CENTER);

#ifdef WITH_LIBHANDY
    gdk_event_handler_set(&MainEventHandler, m_main_window.get(), nullptr);
#endif

    if (!m_settings.IsValid()) {
        Gtk::MessageDialog dlg(*m_main_window, "The settings file could not be opened!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }

    if (!m_emojis.Load()) {
        Gtk::MessageDialog dlg(*m_main_window, "The emoji file couldn't be loaded!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }

    if (!m_discord.IsStoreValid()) {
        Gtk::MessageDialog dlg(*m_main_window, "The Discord cache could not be created!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
        return 1;
    }

#ifdef WITH_VOICE
    if (!m_audio.OK()) {
        Gtk::MessageDialog dlg(*m_main_window, "The audio engine could not be initialized!", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }
#endif

#ifdef _WIN32
    if (m_settings.GetSettings().HideConsole) {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }
#endif

    if (m_settings.GetSettings().FontScale > 0.0) {
        auto dpi = Gdk::Screen::get_default()->get_resolution();
        if (dpi < 0.0) dpi = 96.0;
        auto newdpi = dpi * 1024.0 * m_settings.GetSettings().FontScale;
        Gtk::Settings::get_default()->set_property("gtk-xft-dpi", newdpi);
    }

    // store must be checked before this can be called
    m_main_window->UpdateComponents();

    // crashes for some stupid reason if i put it somewhere else
    SetupUserMenu();

    m_main_window->signal_action_connect().connect(sigc::mem_fun(*this, &Abaddon::ActionConnect));
    m_main_window->signal_action_disconnect().connect(sigc::mem_fun(*this, &Abaddon::ActionDisconnect));
    m_main_window->signal_action_set_token().connect(sigc::mem_fun(*this, &Abaddon::ActionSetToken));
    m_main_window->signal_action_login_qr().connect(sigc::mem_fun(*this, &Abaddon::ActionLoginQR));
    m_main_window->signal_action_reload_css().connect(sigc::mem_fun(*this, &Abaddon::ActionReloadCSS));
    m_main_window->signal_action_set_status().connect(sigc::mem_fun(*this, &Abaddon::ActionSetStatus));
    m_main_window->signal_action_add_recipient().connect(sigc::mem_fun(*this, &Abaddon::ActionAddRecipient));
    m_main_window->signal_action_view_pins().connect(sigc::mem_fun(*this, &Abaddon::ActionViewPins));
    m_main_window->signal_action_view_threads().connect(sigc::mem_fun(*this, &Abaddon::ActionViewThreads));

    m_main_window->GetChannelList()->signal_action_channel_item_select().connect(sigc::bind(sigc::mem_fun(*this, &Abaddon::ActionChannelOpened), true));
    m_main_window->GetChannelList()->signal_action_guild_leave().connect(sigc::mem_fun(*this, &Abaddon::ActionLeaveGuild));
    m_main_window->GetChannelList()->signal_action_guild_settings().connect(sigc::mem_fun(*this, &Abaddon::ActionGuildSettings));

#ifdef WITH_VOICE
    m_main_window->GetChannelList()->signal_action_join_voice_channel().connect(sigc::mem_fun(*this, &Abaddon::ActionJoinVoiceChannel));
    m_main_window->GetChannelList()->signal_action_disconnect_voice().connect(sigc::mem_fun(*this, &Abaddon::ActionDisconnectVoice));
#endif

    m_main_window->GetChatWindow()->signal_action_message_edit().connect(sigc::mem_fun(*this, &Abaddon::ActionChatEditMessage));
    m_main_window->GetChatWindow()->signal_action_chat_submit().connect(sigc::mem_fun(*this, &Abaddon::ActionChatInputSubmit));
    m_main_window->GetChatWindow()->signal_action_chat_load_history().connect(sigc::mem_fun(*this, &Abaddon::ActionChatLoadHistory));
    m_main_window->GetChatWindow()->signal_action_channel_click().connect(sigc::mem_fun(*this, &Abaddon::ActionChannelOpened));
    m_main_window->GetChatWindow()->signal_action_insert_mention().connect(sigc::mem_fun(*this, &Abaddon::ActionInsertMention));
    m_main_window->GetChatWindow()->signal_action_reaction_add().connect(sigc::mem_fun(*this, &Abaddon::ActionReactionAdd));
    m_main_window->GetChatWindow()->signal_action_reaction_remove().connect(sigc::mem_fun(*this, &Abaddon::ActionReactionRemove));

    ActionReloadCSS();
    AttachCSSMonitor();

    if (m_settings.GetSettings().HideToTray) {
        m_tray = Gtk::StatusIcon::create("discord");
        m_tray->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_tray_click));
        m_tray->signal_popup_menu().connect(sigc::mem_fun(*this, &Abaddon::on_tray_popup_menu));
    }
    m_tray_menu = Gtk::make_managed<Gtk::Menu>();
    m_tray_exit = Gtk::make_managed<Gtk::MenuItem>("Quit", false);

    m_tray_exit->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_tray_menu_click));

    m_tray_menu->append(*m_tray_exit);
    m_tray_menu->show_all();

    m_main_window->signal_hide().connect(sigc::mem_fun(*this, &Abaddon::on_window_hide));
    m_gtk_app->signal_shutdown().connect(sigc::mem_fun(*this, &Abaddon::OnShutdown), false);

    m_main_window->UpdateMenus();

    auto action_go_to_channel = Gio::SimpleAction::create("go-to-channel", Glib::VariantType("s"));
    action_go_to_channel->signal_activate().connect([this](const Glib::VariantBase &param) {
        const auto id_str = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(param);
        const Snowflake id = id_str.get();
        ActionChannelOpened(id, false);
    });
    m_gtk_app->add_action(action_go_to_channel);

    m_gtk_app->hold();
    m_main_window->show();

#if defined(__APPLE__)
    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                    NULL,
                                    macOSThemeChangedCallback,
                                    CFSTR("AppleInterfaceThemeChangedNotification"),
                                    NULL,
                                    CFNotificationSuspensionBehaviorCoalesce);

    macOSThemeChanged();
#endif

    RunFirstTimeDiscordStartup();

    return m_gtk_app->run(*m_main_window);
}

void Abaddon::OnShutdown() {
    StopDiscord();
    m_settings.Close();
}

void Abaddon::LoadFromSettings() {
    std::string token = GetSettings().DiscordToken;
    if (!token.empty()) {
        m_discord_token = token;
        m_discord.UpdateToken(m_discord_token);
    }
}

void Abaddon::StartDiscord() {
    m_discord.Start();
    m_main_window->UpdateMenus();
}

void Abaddon::StopDiscord() {
    if (m_discord.IsStarted()) SaveState();
    m_discord.Stop();
    m_main_window->UpdateMenus();
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
    LoadState();
}

void Abaddon::DiscordOnMessageCreate(const Message &message) {
    m_main_window->UpdateChatNewMessage(message);
    m_notifications.CheckMessage(message);
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

void Abaddon::DiscordOnThreadMemberListUpdate(const ThreadMemberListUpdateData &data) {
    m_main_window->UpdateMembers();
}

void Abaddon::DiscordOnReactionAdd(Snowflake message_id, const Glib::ustring &param) {
    m_main_window->UpdateChatReactionAdd(message_id, param);
}

void Abaddon::DiscordOnReactionRemove(Snowflake message_id, const Glib::ustring &param) {
    m_main_window->UpdateChatReactionAdd(message_id, param);
}

// this will probably cause issues when actual applications are rolled out but that doesn't seem like it will happen for a while
void Abaddon::DiscordOnGuildJoinRequestCreate(const GuildJoinRequestCreateData &data) {
    if (data.Status == GuildApplicationStatus::STARTED) {
        ShowGuildVerificationGateDialog(data.GuildID);
    }
}

void Abaddon::DiscordOnMessageSent(const Message &data) {
    m_main_window->UpdateChatNewMessage(data);
}

void Abaddon::DiscordOnDisconnect(bool is_reconnecting, GatewayCloseCode close_code) {
    m_channels_history_loaded.clear();
    m_channels_history_loading.clear();
    m_channels_requested.clear();
    if (is_reconnecting) return;
    m_main_window->set_title(APP_TITLE);
    m_main_window->UpdateComponents();

    if (close_code == GatewayCloseCode::AuthenticationFailed) {
        Gtk::MessageDialog dlg(*m_main_window, "Discord rejected your token", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    } else if (close_code != GatewayCloseCode::Normal) {
        Gtk::MessageDialog dlg(*m_main_window,
                               "Lost connection with Discord's gateway. Try reconnecting (code " + std::to_string(static_cast<unsigned>(close_code)) + ")",
                               false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }
}

void Abaddon::DiscordOnThreadUpdate(const ThreadUpdateData &data) {
    if (data.Thread.ID == m_main_window->GetChatActiveChannel()) {
        if (data.Thread.ThreadMetadata->IsArchived)
            m_main_window->GetChatWindow()->SetTopic("This thread is archived. Sending a message will unarchive it");
        else
            m_main_window->GetChatWindow()->SetTopic("");
    }
}

#ifdef WITH_VOICE
void Abaddon::OnVoiceConnected() {
    m_audio.StartCaptureDevice();
    ShowVoiceWindow();
}

void Abaddon::OnVoiceDisconnected() {
    m_audio.StopCaptureDevice();
    m_audio.RemoveAllSSRCs();
    if (m_voice_window != nullptr) {
        m_voice_window->close();
    }
}

void Abaddon::ShowVoiceWindow() {
    if (m_voice_window != nullptr) return;

    auto *wnd = new VoiceWindow(m_discord.GetVoiceChannelID());
    m_voice_window = wnd;

    wnd->signal_mute().connect([this](bool is_mute) {
        m_discord.SetVoiceMuted(is_mute);
        m_audio.SetCapture(!is_mute);
    });

    wnd->signal_deafen().connect([this](bool is_deaf) {
        m_discord.SetVoiceDeafened(is_deaf);
        m_audio.SetPlayback(!is_deaf);
    });

    wnd->signal_mute_user_cs().connect([this](Snowflake id, bool is_mute) {
        if (const auto ssrc = m_discord.GetSSRCOfUser(id); ssrc.has_value()) {
            m_audio.SetMuteSSRC(*ssrc, is_mute);
        }
    });

    wnd->signal_user_volume_changed().connect([this](Snowflake id, double volume) {
        auto &vc = m_discord.GetVoiceClient();
        vc.SetUserVolume(id, volume);
    });

    wnd->set_position(Gtk::WIN_POS_CENTER);

    wnd->show();
    wnd->signal_hide().connect([this, wnd]() {
        m_voice_window = nullptr;
        delete wnd;
        delete m_user_menu;
        SetupUserMenu();
    });
}
#endif

SettingsManager::Settings &Abaddon::GetSettings() {
    return m_settings.GetSettings();
}

Glib::RefPtr<Gtk::CssProvider> Abaddon::GetStyleProvider() {
    return m_css_provider;
}

void Abaddon::ShowUserMenu(const GdkEvent *event, Snowflake id, Snowflake guild_id) {
    m_shown_user_menu_id = id;
    m_shown_user_menu_guild_id = guild_id;

    const auto guild = m_discord.GetGuild(guild_id);
    const auto me = m_discord.GetUserData().ID;
    const auto user = m_discord.GetMember(id, guild_id);

    for (const auto child : m_user_menu_roles_submenu->get_children())
        delete child;

    if (guild.has_value() && user.has_value()) {
        const auto roles = user->GetSortedRoles();
        m_user_menu_roles->set_visible(!roles.empty());
        for (const auto &role : roles) {
            auto *item = Gtk::manage(new Gtk::MenuItem(role.Name));
            if (role.Color != 0) {
                Gdk::RGBA color;
                color.set_red(((role.Color & 0xFF0000) >> 16) / 255.0);
                color.set_green(((role.Color & 0x00FF00) >> 8) / 255.0);
                color.set_blue(((role.Color & 0x0000FF) >> 0) / 255.0);
                color.set_alpha(1.0);
                item->override_color(color);
            }
            item->show();
            m_user_menu_roles_submenu->append(*item);
        }
    } else
        m_user_menu_roles->set_visible(false);

    if (me == id) {
        m_user_menu_ban->set_visible(false);
        m_user_menu_kick->set_visible(false);
        m_user_menu_open_dm->set_sensitive(false);
    } else {
        const bool has_kick = m_discord.HasGuildPermission(me, guild_id, Permission::KICK_MEMBERS);
        const bool has_ban = m_discord.HasGuildPermission(me, guild_id, Permission::BAN_MEMBERS);
        const bool can_manage = m_discord.CanManageMember(guild_id, me, id);

        m_user_menu_kick->set_visible(has_kick && can_manage);
        m_user_menu_ban->set_visible(has_ban && can_manage);
        m_user_menu_open_dm->set_sensitive(m_discord.FindDM(id).has_value());
    }

    m_user_menu_remove_recipient->hide();
    if (me != id) {
        const auto channel_id = m_main_window->GetChatActiveChannel();
        const auto channel = m_discord.GetChannel(channel_id);
        if (channel.has_value() && channel->Type == ChannelType::GROUP_DM && me == *channel->OwnerID)
            m_user_menu_remove_recipient->show();
    }

    m_user_menu->popup_at_pointer(event);
}

void Abaddon::RunFirstTimeDiscordStartup() {
    DiscordStartupDialog dlg(*m_main_window);
    dlg.set_position(Gtk::WIN_POS_CENTER);

    std::optional<std::string> cookie;
    std::optional<uint32_t> build_number;

    dlg.signal_response().connect([&](int response) {
        if (response == Gtk::RESPONSE_OK) {
            cookie = dlg.GetCookie();
            build_number = dlg.GetBuildNumber();
        }
    });

    dlg.run();

    Glib::signal_idle().connect_once([this, cookie, build_number]() {
        if (cookie.has_value()) {
            m_discord.SetCookie(*cookie);
        } else {
            ConfirmDialog confirm(*m_main_window);
            confirm.SetConfirmText("Cookies could not be fetched. This may increase your chances of being flagged by Discord's anti-spam");
            confirm.SetAcceptOnly(true);
            confirm.run();
        }

        if (build_number.has_value()) {
            m_discord.SetBuildNumber(*build_number);
        } else {
            ConfirmDialog confirm(*m_main_window);
            confirm.SetConfirmText("Build number could not be fetched. This may increase your chances of being flagged by Discord's anti-spam");
            confirm.SetAcceptOnly(true);
            confirm.run();
        }

        // autoconnect
        if (cookie.has_value() && build_number.has_value() && GetSettings().Autoconnect && !GetDiscordToken().empty()) {
            ActionConnect();
        }
    });
}

void Abaddon::ShowGuildVerificationGateDialog(Snowflake guild_id) {
    VerificationGateDialog dlg(*m_main_window, guild_id);
    if (dlg.run() == Gtk::RESPONSE_OK) {
        const auto cb = [this](DiscordError code) {
            if (code != DiscordError::NONE) {
                Gtk::MessageDialog dlg(*m_main_window, "Failed to accept the verification gate.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
                dlg.set_position(Gtk::WIN_POS_CENTER);
                dlg.run();
            }
        };
        m_discord.AcceptVerificationGate(guild_id, dlg.GetVerificationGate(), cb);
    }
}

void Abaddon::CheckMessagesForMembers(const ChannelData &chan, const std::vector<Message> &msgs) {
    if (!chan.GuildID.has_value()) return;

    std::vector<Snowflake> unknown;
    std::transform(msgs.begin(), msgs.end(),
                   std::back_inserter(unknown),
                   [](const Message &msg) -> Snowflake {
                       return msg.Author.ID;
                   });

    const auto fetch = m_discord.FilterUnknownMembersFrom(*chan.GuildID, unknown.begin(), unknown.end());
    m_discord.RequestMembers(*chan.GuildID, fetch.begin(), fetch.end());
}

void Abaddon::SetupUserMenu() {
    m_user_menu = Gtk::manage(new Gtk::Menu);
    m_user_menu_insert_mention = Gtk::manage(new Gtk::MenuItem("Insert Mention"));
    m_user_menu_ban = Gtk::manage(new Gtk::MenuItem("Ban"));
    m_user_menu_kick = Gtk::manage(new Gtk::MenuItem("Kick"));
    m_user_menu_copy_id = Gtk::manage(new Gtk::MenuItem("Copy ID"));
    m_user_menu_open_dm = Gtk::manage(new Gtk::MenuItem("Go to DM"));
    m_user_menu_roles = Gtk::manage(new Gtk::MenuItem("Roles"));
    m_user_menu_info = Gtk::manage(new Gtk::MenuItem("View Profile"));
    m_user_menu_remove_recipient = Gtk::manage(new Gtk::MenuItem("Remove From Group"));
    m_user_menu_roles_submenu = Gtk::manage(new Gtk::Menu);
    m_user_menu_roles->set_submenu(*m_user_menu_roles_submenu);
    m_user_menu_insert_mention->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_insert_mention));
    m_user_menu_ban->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_ban));
    m_user_menu_kick->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_kick));
    m_user_menu_copy_id->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_copy_id));
    m_user_menu_open_dm->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_open_dm));
    m_user_menu_remove_recipient->signal_activate().connect(sigc::mem_fun(*this, &Abaddon::on_user_menu_remove_recipient));
    m_user_menu_info->signal_activate().connect([this]() {
        auto *window = new ProfileWindow(m_shown_user_menu_id);
        ManageHeapWindow(window);
        window->show();
    });

    m_user_menu_remove_recipient->override_color(Gdk::RGBA("#BE3C3D"));

    m_user_menu->append(*m_user_menu_info);
    m_user_menu->append(*m_user_menu_insert_mention);
    m_user_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem));
    m_user_menu->append(*m_user_menu_ban);
    m_user_menu->append(*m_user_menu_kick);
    m_user_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem));
    m_user_menu->append(*m_user_menu_open_dm);
    m_user_menu->append(*m_user_menu_roles);
    m_user_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem));
    m_user_menu->append(*m_user_menu_remove_recipient);
    m_user_menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem));
    m_user_menu->append(*m_user_menu_copy_id);

    m_user_menu->show_all();
}

void Abaddon::SaveState() {
    if (!GetSettings().SaveState) return;

    AbaddonApplicationState state;
    state.ActiveChannel = m_main_window->GetChatActiveChannel();
    state.Expansion = m_main_window->GetChannelList()->GetExpansionState();
#ifdef WITH_LIBHANDY
    state.Tabs = m_main_window->GetChatWindow()->GetTabsState();
#endif

    const auto path = GetStateCachePath();
    if (!util::IsFolder(path)) {
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
    }

    auto file_name = "/" + std::to_string(m_discord.GetUserData().ID) + ".json";
    auto *fp = std::fopen(GetStateCachePath(file_name).c_str(), "wb");
    if (fp == nullptr) return;
    const auto s = nlohmann::json(state).dump(4);
    std::fwrite(s.c_str(), 1, s.size(), fp);
    std::fclose(fp);
}

void Abaddon::LoadState() {
    if (!GetSettings().SaveState) {
        // call with empty data to purge the temporary table
        m_main_window->GetChannelList()->UseExpansionState({});
        return;
    }

    auto file_name = "/" + std::to_string(m_discord.GetUserData().ID) + ".json";
    const auto data = ReadWholeFile(GetStateCachePath(file_name));
    if (data.empty()) return;
    try {
        AbaddonApplicationState state = nlohmann::json::parse(data.begin(), data.end());
        m_main_window->GetChannelList()->UseExpansionState(state.Expansion);
#ifdef WITH_LIBHANDY
        m_main_window->GetChatWindow()->UseTabsState(state.Tabs);
#endif
        ActionChannelOpened(state.ActiveChannel, false);
    } catch (const std::exception &e) {
        printf("failed to load application state: %s\n", e.what());
    }
}

void Abaddon::AttachCSSMonitor() {
    const auto path = GetCSSPath("/" + GetSettings().MainCSS);
    const auto file = Gio::File::create_for_path(path);
    m_main_css_monitor = file->monitor_file();
    m_main_css_monitor->signal_changed().connect([this](const auto &file, const auto &other_file, Gio::FileMonitorEvent event) {
        ActionReloadCSS();
    });
}

void Abaddon::ManageHeapWindow(Gtk::Window *window) {
    window->signal_hide().connect([this, window]() {
        delete window;
        // for some reason if ShowUserMenu is called multiple times with events across windows
        // and one of the windows is closed, then it throws errors when the menu is opened again
        // i dont think this is my fault so this semi-hacky solution will have to do
        delete m_user_menu;
        SetupUserMenu();
    });
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
    if (existing.has_value()) {
        ActionChannelOpened(*existing);
    }
}

void Abaddon::on_user_menu_remove_recipient() {
    m_discord.RemoveGroupDMRecipient(m_main_window->GetChatActiveChannel(), m_shown_user_menu_id);
}

std::string Abaddon::GetCSSPath() {
    const static auto path = Platform::FindResourceFolder() + "/css";
    return path;
}

std::string Abaddon::GetResPath() {
    const static auto path = Platform::FindResourceFolder() + "/res";
    return path;
}

std::string Abaddon::GetStateCachePath() {
    const static auto path = Platform::FindStateCacheFolder() + "/state";
    return path;
}

std::string Abaddon::GetCSSPath(const std::string &path) {
    return GetCSSPath() + path;
}

std::string Abaddon::GetResPath(const std::string &path) {
    return GetResPath() + path;
}

std::string Abaddon::GetStateCachePath(const std::string &path) {
    return GetStateCachePath() + path;
}

Glib::RefPtr<Gtk::Application> Abaddon::GetApp() {
    return m_gtk_app;
}

bool Abaddon::IsMainWindowActive() {
    return m_main_window->has_toplevel_focus();
}

Snowflake Abaddon::GetActiveChannelID() const noexcept {
    return m_main_window->GetChatActiveChannel();
}

void Abaddon::ActionConnect() {
    if (!m_discord.IsStarted())
        StartDiscord();
    m_main_window->UpdateComponents();
}

void Abaddon::ActionDisconnect() {
    StopDiscord();
}

void Abaddon::ActionSetToken() {
    TokenDialog dlg(*m_main_window);
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK) {
        m_discord_token = dlg.GetToken();
        m_discord.UpdateToken(m_discord_token);
        m_main_window->UpdateComponents();
        GetSettings().DiscordToken = m_discord_token;
    }
    m_main_window->UpdateMenus();
}

void Abaddon::ActionLoginQR() {
#ifdef WITH_QRLOGIN
    RemoteAuthDialog dlg(*m_main_window);
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK) {
        m_discord_token = dlg.GetToken();
        m_discord.UpdateToken(m_discord_token);
        m_main_window->UpdateComponents();
        GetSettings().DiscordToken = m_discord_token;
        ActionConnect();
    }
    m_main_window->UpdateMenus();
#endif
}

void Abaddon::ActionChannelOpened(Snowflake id, bool expand_to) {
    if (!id.IsValid()) {
        m_discord.SetReferringChannel(Snowflake::Invalid);
        return;
    }
    if (id == m_main_window->GetChatActiveChannel()) return;

    m_notifications.WithdrawChannel(id);

    m_main_window->GetChatWindow()->SetTopic("");

    const auto channel = m_discord.GetChannel(id);
    if (!channel.has_value()) {
        m_discord.SetReferringChannel(Snowflake::Invalid);
        m_main_window->UpdateChatActiveChannel(Snowflake::Invalid, false);
        m_main_window->UpdateChatWindowContents();
        return;
    }

    const bool can_access = channel->IsDM() || m_discord.HasChannelPermission(m_discord.GetUserData().ID, id, Permission::VIEW_CHANNEL);

    m_main_window->set_title(std::string(APP_TITLE) + " - " + channel->GetDisplayName());
    m_main_window->UpdateChatActiveChannel(id, expand_to);
    if (m_channels_requested.find(id) == m_channels_requested.end()) {
        // dont fire requests we know will fail
        if (can_access) {
            m_discord.FetchMessagesInChannel(id, [channel, this, id](const std::vector<Message> &msgs) {
                CheckMessagesForMembers(*channel, msgs);
                m_main_window->UpdateChatWindowContents();
                m_channels_requested.insert(id);
            });
        }
    } else {
        m_main_window->UpdateChatWindowContents();
    }

    if (can_access) {
        if (channel->IsThread()) {
            m_discord.SendThreadLazyLoad(id);
            if (channel->ThreadMetadata->IsArchived)
                m_main_window->GetChatWindow()->SetTopic("This thread is archived. Sending a message will unarchive it");
        } else if (channel->Type != ChannelType::DM && channel->Type != ChannelType::GROUP_DM && channel->GuildID.has_value()) {
            m_discord.SendLazyLoad(id);

            if (m_discord.IsVerificationRequired(*channel->GuildID))
                ShowGuildVerificationGateDialog(*channel->GuildID);
        }
    }

    m_main_window->UpdateMenus();
    m_discord.SetReferringChannel(id);
}

void Abaddon::ActionChatLoadHistory(Snowflake id) {
    if (m_channels_history_loaded.find(id) != m_channels_history_loaded.end())
        return;

    if (m_channels_history_loading.find(id) != m_channels_history_loading.end())
        return;

    Snowflake before_id = m_main_window->GetChatOldestListedMessage();
    auto msgs = m_discord.GetMessagesBefore(id, before_id);

    if (msgs.size() >= 50) {
        m_main_window->UpdateChatPrependHistory(msgs);
        return;
    }

    m_channels_history_loading.insert(id);

    m_discord.FetchMessagesInChannelBefore(id, before_id, [this, id](const std::vector<Message> &msgs) {
        m_channels_history_loading.erase(id);

        const auto channel = m_discord.GetChannel(id);
        if (channel.has_value())
            CheckMessagesForMembers(*channel, msgs);

        if (msgs.empty()) {
            m_channels_history_loaded.insert(id);
        } else {
            m_main_window->UpdateChatPrependHistory(msgs);
        }
    });
}

void Abaddon::ActionChatInputSubmit(ChatSubmitParams data) {
    if (data.Message.substr(0, 7) == "/shrug " || data.Message == "/shrug") {
        data.Message = data.Message.substr(6) + "\xC2\xAF\x5C\x5F\x28\xE3\x83\x84\x29\x5F\x2F\xC2\xAF"; // this is important
    }

    if (data.Message.substr(0, 8) == "@silent " || (data.Message.substr(0, 7) == "@silent" && !data.Attachments.empty())) {
        data.Silent = true;
        data.Message = data.Message.substr(7);
    }

    if (!m_discord.HasChannelPermission(m_discord.GetUserData().ID, data.ChannelID, Permission::VIEW_CHANNEL)) return;

    if (data.EditingID.IsValid()) {
        m_discord.EditMessage(data.ChannelID, data.EditingID, data.Message);
    } else {
        m_discord.SendChatMessage(data, NOOP_CALLBACK);
    }
}

void Abaddon::ActionChatEditMessage(Snowflake channel_id, Snowflake id) {
    m_main_window->EditMessage(id);
}

void Abaddon::ActionInsertMention(Snowflake id) {
    m_main_window->InsertChatInput("<@" + std::to_string(id) + ">");
}

void Abaddon::ActionLeaveGuild(Snowflake id) {
    ConfirmDialog dlg(*m_main_window);
    const auto guild = m_discord.GetGuild(id);
    if (guild.has_value())
        dlg.SetConfirmText("Are you sure you want to leave " + guild->Name + "?");
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK)
        m_discord.LeaveGuild(id);
}

void Abaddon::ActionKickMember(Snowflake user_id, Snowflake guild_id) {
    ConfirmDialog dlg(*m_main_window);
    const auto user = m_discord.GetUser(user_id);
    if (user.has_value())
        dlg.SetConfirmText("Are you sure you want to kick " + user->GetUsername() + "?");
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK)
        m_discord.KickUser(user_id, guild_id);
}

void Abaddon::ActionBanMember(Snowflake user_id, Snowflake guild_id) {
    ConfirmDialog dlg(*m_main_window);
    const auto user = m_discord.GetUser(user_id);
    if (user.has_value())
        dlg.SetConfirmText("Are you sure you want to ban " + user->GetUsername() + "?");
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
    if (activity_name.empty()) {
        m_discord.UpdateStatus(status, false);
    } else {
        ActivityData activity;
        activity.Name = activity_name;
        activity.Type = activity_type;
        m_discord.UpdateStatus(status, false, activity);
    }
}

void Abaddon::ActionReactionAdd(Snowflake id, const Glib::ustring &param) {
    m_discord.AddReaction(id, param);
}

void Abaddon::ActionReactionRemove(Snowflake id, const Glib::ustring &param) {
    m_discord.RemoveReaction(id, param);
}

void Abaddon::ActionGuildSettings(Snowflake id) {
    auto window = new GuildSettingsWindow(id);
    ManageHeapWindow(window);
    window->show();
}

void Abaddon::ActionAddRecipient(Snowflake channel_id) {
    FriendPickerDialog dlg(*m_main_window);
    auto response = dlg.run();
    if (response == Gtk::RESPONSE_OK) {
        auto user_id = dlg.GetUserID();
        m_discord.AddGroupDMRecipient(channel_id, user_id);
    }
}

void Abaddon::ActionViewPins(Snowflake channel_id) {
    const auto data = m_discord.GetChannel(channel_id);
    if (!data.has_value()) return;
    auto window = new PinnedWindow(*data);
    ManageHeapWindow(window);
    window->show();
}

void Abaddon::ActionViewThreads(Snowflake channel_id) {
    auto data = m_discord.GetChannel(channel_id);
    if (!data.has_value()) return;
    if (data->IsThread()) {
        data = m_discord.GetChannel(*data->ParentID);
        if (!data.has_value()) return;
    }
    auto window = new ThreadsWindow(*data);
    ManageHeapWindow(window);
    window->show();
}

#ifdef WITH_VOICE
void Abaddon::ActionJoinVoiceChannel(Snowflake channel_id) {
    m_discord.ConnectToVoice(channel_id);
}

void Abaddon::ActionDisconnectVoice() {
    m_discord.DisconnectFromVoice();
}
#endif

std::optional<Glib::ustring> Abaddon::ShowTextPrompt(const Glib::ustring &prompt, const Glib::ustring &title, const Glib::ustring &placeholder, Gtk::Window *window) {
    TextInputDialog dlg(prompt, title, placeholder, window != nullptr ? *window : *m_main_window);
    const auto code = dlg.run();
    if (code == Gtk::RESPONSE_OK)
        return dlg.GetInput();
    else
        return {};
}

bool Abaddon::ShowConfirm(const Glib::ustring &prompt, Gtk::Window *window) {
    ConfirmDialog dlg(window != nullptr ? *window : *m_main_window);
    dlg.SetConfirmText(prompt);
    return dlg.run() == Gtk::RESPONSE_OK;
}

void Abaddon::ActionReloadCSS() {
    try {
        Gtk::StyleContext::remove_provider_for_screen(Gdk::Screen::get_default(), m_css_provider);
        m_css_provider->load_from_path(GetCSSPath("/" + GetSettings().MainCSS));
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), m_css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    } catch (Glib::Error &e) {
        Gtk::MessageDialog dlg(*m_main_window, "css failed to load (" + e.what() + ")", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        dlg.set_position(Gtk::WIN_POS_CENTER);
        dlg.run();
    }
}

ImageManager &Abaddon::GetImageManager() {
    return m_img_mgr;
}

EmojiResource &Abaddon::GetEmojis() {
    return m_emojis;
}

#ifdef WITH_VOICE
AudioManager &Abaddon::GetAudio() {
    return m_audio;
}
#endif

void Abaddon::on_tray_click() {
    m_main_window->set_visible(!m_main_window->is_visible());
}

void Abaddon::on_tray_menu_click() {
    m_gtk_app->quit();
}

void Abaddon::on_tray_popup_menu(int button, int activate_time) {
    m_tray->popup_menu_at_position(*m_tray_menu, button, activate_time);
}

void Abaddon::on_window_hide() {
    if (!m_settings.GetSettings().HideToTray) {
        m_gtk_app->quit();
    }
}

int main(int argc, char **argv) {
    if (std::getenv("ABADDON_NO_FC") == nullptr)
        Platform::SetupFonts();

    char *systemLocale = std::setlocale(LC_ALL, "");
    try {
        if (systemLocale != nullptr) {
            std::locale::global(std::locale(systemLocale));
        }
    } catch (...) {
        try {
            std::locale::global(std::locale::classic());
            if (systemLocale != nullptr) {
                std::setlocale(LC_ALL, systemLocale);
            }
        } catch (...) {}
    }

#if defined(_WIN32) && defined(_MSC_VER)
    TCHAR buf[2] { 0 };
    GetEnvironmentVariableA("GTK_CSD", buf, sizeof(buf));
    if (buf[0] != '1')
        SetEnvironmentVariableA("GTK_CSD", "0");
#endif

    spdlog::cfg::load_env_levels();
    auto log_ui = spdlog::stdout_color_mt("ui");
    auto log_audio = spdlog::stdout_color_mt("audio");
    auto log_voice = spdlog::stdout_color_mt("voice");
    auto log_discord = spdlog::stdout_color_mt("discord");
    auto log_ra = spdlog::stdout_color_mt("remote-auth");

    Gtk::Main::init_gtkmm_internals(); // why???
    return Abaddon::Get().StartGTK();
}
