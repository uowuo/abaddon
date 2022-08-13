#include "settings.hpp"
#include <filesystem>
#include <fstream>

SettingsManager::SettingsManager(const std::string &filename)
    : m_filename(filename) {
    if (!std::filesystem::exists(filename)) {
        std::fstream fs;
        fs.open(filename, std::ios::out);
        fs.close();
    }

    try {
        m_ok = m_file.load_from_file(m_filename, Glib::KEY_FILE_KEEP_COMMENTS);
    } catch (const Glib::Error &e) {
        fprintf(stderr, "error opening settings KeyFile: %s\n", e.what().c_str());
        m_ok = false;
    }

    if (m_ok) ReadSettings();
}

void SettingsManager::ReadSettings() {
#define SMBOOL(section, key, var)                          \
    try {                                                  \
        m_settings.var = m_file.get_boolean(section, key); \
    } catch (...) {}
#define SMSTR(section, key, var)                          \
    try {                                                 \
        m_settings.var = m_file.get_string(section, key); \
    } catch (...) {}
#define SMINT(section, key, var)                           \
    try {                                                  \
        m_settings.var = m_file.get_integer(section, key); \
    } catch (...) {}

    SMSTR("discord", "api_base", APIBaseURL);
    SMSTR("discord", "gateway", GatewayURL);
    SMSTR("discord", "token", DiscordToken);
    SMBOOL("discord", "memory_db", UseMemoryDB);
    SMBOOL("discord", "prefetch", Prefetch);
    SMSTR("gui", "css", MainCSS);
    SMBOOL("gui", "animated_guild_hover_only", AnimatedGuildHoverOnly);
    SMBOOL("gui", "animations", ShowAnimations);
    SMBOOL("gui", "custom_emojis", ShowCustomEmojis);
    SMBOOL("gui", "member_list_discriminator", ShowMemberListDiscriminators);
    SMBOOL("gui", "owner_crown", ShowOwnerCrown);
    SMBOOL("gui", "save_state", SaveState);
    SMBOOL("gui", "stock_emojis", ShowStockEmojis);
    SMBOOL("gui", "unreads", Unreads);
    SMBOOL("gui", "hide_to_tray", HideToTray);
    SMINT("http", "concurrent", CacheHTTPConcurrency);
    SMSTR("http", "user_agent", UserAgent);
    SMSTR("style", "expandercolor", ChannelsExpanderColor);
    SMSTR("style", "linkcolor", LinkColor);
    SMSTR("style", "nsfwchannelcolor", NSFWChannelColor);
    SMSTR("style", "channelcolor", ChannelColor);
    SMSTR("style", "mentionbadgecolor", MentionBadgeColor);
    SMSTR("style", "mentionbadgetextcolor", MentionBadgeTextColor);
    SMSTR("style", "unreadcolor", UnreadIndicatorColor);

#undef SMBOOL
#undef SMSTR
#undef SMINT

    m_read_settings = m_settings;
}

bool SettingsManager::IsValid() const {
    return m_ok;
}

SettingsManager::Settings &SettingsManager::GetSettings() {
    return m_settings;
}

void SettingsManager::Close() {
    if (m_ok) {
        // save anything that changed
        // (futureproofing since only DiscordToken can actually change)
#define SMSTR(section, key, var)               \
    if (m_settings.var != m_read_settings.var) \
        m_file.set_string(section, key, m_settings.var);
#define SMBOOL(section, key, var)              \
    if (m_settings.var != m_read_settings.var) \
        m_file.set_boolean(section, key, m_settings.var);
#define SMINT(section, key, var)               \
    if (m_settings.var != m_read_settings.var) \
        m_file.set_integer(section, key, m_settings.var);

        SMSTR("discord", "api_base", APIBaseURL);
        SMSTR("discord", "gateway", GatewayURL);
        SMSTR("discord", "token", DiscordToken);
        SMBOOL("discord", "memory_db", UseMemoryDB);
        SMBOOL("discord", "prefetch", Prefetch);
        SMSTR("gui", "css", MainCSS);
        SMBOOL("gui", "animated_guild_hover_only", AnimatedGuildHoverOnly);
        SMBOOL("gui", "animations", ShowAnimations);
        SMBOOL("gui", "custom_emojis", ShowCustomEmojis);
        SMBOOL("gui", "member_list_discriminator", ShowMemberListDiscriminators);
        SMBOOL("gui", "owner_crown", ShowOwnerCrown);
        SMBOOL("gui", "save_state", SaveState);
        SMBOOL("gui", "stock_emojis", ShowStockEmojis);
        SMBOOL("gui", "unreads", Unreads);
        SMBOOL("gui", "hide_to_tray", HideToTray);
        SMINT("http", "concurrent", CacheHTTPConcurrency);
        SMSTR("http", "user_agent", UserAgent);
        SMSTR("style", "expandercolor", ChannelsExpanderColor);
        SMSTR("style", "linkcolor", LinkColor);
        SMSTR("style", "nsfwchannelcolor", NSFWChannelColor);
        SMSTR("style", "channelcolor", ChannelColor);
        SMSTR("style", "mentionbadgecolor", MentionBadgeColor);
        SMSTR("style", "mentionbadgetextcolor", MentionBadgeTextColor);
        SMSTR("style", "unreadcolor", UnreadIndicatorColor);

#undef SMSTR
#undef SMBOOL
#undef SMINT

        try {
            if (!m_file.save_to_file(m_filename))
                fputs("failed to save settings KeyFile", stderr);
        } catch (const Glib::Error &e) {
            fprintf(stderr, "failed to save settings KeyFile: %s\n", e.what().c_str());
        }
    }
}
