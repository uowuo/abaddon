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

        bool HideToTray { false };

        // [http]
        int CacheHTTPConcurrency { 20 };
        std::string UserAgent { "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36" };

        // [style]
        // TODO: convert to StyleProperty... or maybe not? i still cant figure out what the "correct" method is for this
        std::string LinkColor { "rgba(40, 200, 180, 255)" };
        std::string ChannelsExpanderColor { "rgba(255, 83, 112, 255)" };
        std::string NSFWChannelColor { "#ed6666" };
        std::string ChannelColor { "#fbfbfb" };
        std::string MentionBadgeColor { "#b82525" };
        std::string MentionBadgeTextColor { "#fbfbfb" };
        std::string UnreadIndicatorColor { "#ffffff" };
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
