#pragma once
#include <string>
#include <type_traits>
#include <glibmm/keyfile.h>

class SettingsManager {
public:
    struct Settings {
        // [discord]
        std::string APIBaseURL { "https://discord.com/api/v9" };
        std::string GatewayURL { "wss://gateway.discord.gg/?v=9&encoding=json&compress=zlib-stream" };
        std::string DiscordToken;
        bool UseMemoryDB { false };
        bool Prefetch { false };
        bool Autoconnect { false };

        // [gui]
        std::string MainCSS { "main.css" };
        bool AnimatedGuildHoverOnly { true };
        bool ShowAnimations { true };
        bool ShowCustomEmojis { true };
        bool ShowMemberListDiscriminators { true };
        bool ShowOwnerCrown { true };
        bool SaveState { true };
#ifdef _WIN32
        bool ShowStockEmojis { false };
#else
        bool ShowStockEmojis { true };
#endif
        bool Unreads { true };
        bool AltMenu { false };
        bool HideToTray { false };
        bool ShowDeletedIndicator { true };
        double FontScale { -1.0 };

        // [http]
        int CacheHTTPConcurrency { 20 };
        std::string UserAgent { "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36" };

        // [style]
        std::string ChannelsExpanderColor { "rgba(255, 83, 112, 0)" };
        std::string NSFWChannelColor { "#970d0d" };
        std::string MentionBadgeColor { "rgba(184, 37, 37, 0)" };
        std::string MentionBadgeTextColor { "rgba(251, 251, 251, 0)" };
        std::string UnreadIndicatorColor { "rgba(255, 255, 255, 0)" };

        // [notifications]
#ifdef _WIN32
        bool NotificationsEnabled { false };
#else
        bool NotificationsEnabled { true };
#endif
        bool NotificationsPlaySound { true };

        // [voice]
#ifdef WITH_RNNOISE
        std::string VAD { "rnnoise" };
#else
        std::string VAD { "gate" };
#endif

        // [windows]
        bool HideConsole { false };
    };

    SettingsManager(const std::string &filename);

    void Close();
    [[nodiscard]] bool IsValid() const;
    Settings &GetSettings();

private:
    void ReadSettings();

    bool m_ok;
    std::string m_filename;
    Glib::KeyFile m_file;
    Settings m_settings;
    Settings m_read_settings;
};
