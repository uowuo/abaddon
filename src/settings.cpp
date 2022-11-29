#include "settings.hpp"
#include <filesystem>
#include <fstream>
#include <glibmm/miscutils.h>

#ifdef WITH_KEYCHAIN
    #include <keychain/keychain.h>
#endif

const std::string KeychainPackage = "com.github.uowuo.abaddon";
const std::string KeychainService = "abaddon-client-token";
const std::string KeychainUser = "";

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
    SMBOOL("discord", "memory_db", UseMemoryDB);
    SMBOOL("discord", "prefetch", Prefetch);
    SMBOOL("discord", "autoconnect", Autoconnect);
    SMSTR("gui", "css", MainCSS);
    SMBOOL("gui", "animated_guild_hover_only", AnimatedGuildHoverOnly);
    SMBOOL("gui", "animations", ShowAnimations);
    SMBOOL("gui", "custom_emojis", ShowCustomEmojis);
    SMBOOL("gui", "member_list_discriminator", ShowMemberListDiscriminators);
    SMBOOL("gui", "owner_crown", ShowOwnerCrown);
    SMBOOL("gui", "save_state", SaveState);
    SMBOOL("gui", "stock_emojis", ShowStockEmojis);
    SMBOOL("gui", "unreads", Unreads);
    SMBOOL("gui", "alt_menu", AltMenu);
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

#ifdef WITH_KEYCHAIN
    keychain::Error error {};

    // convert to keychain if present in normal settings
    SMSTR("discord", "token", DiscordToken);

    if (!m_settings.DiscordToken.empty()) {
        keychain::Error set_error {};

        keychain::setPassword(KeychainPackage, KeychainService, KeychainUser, m_settings.DiscordToken, set_error);
        if (set_error) {
            printf("keychain error setting token: %s\n", set_error.message.c_str());
        } else {
            m_file.remove_key("discord", "token");
        }
    }

    m_settings.DiscordToken = keychain::getPassword(KeychainPackage, KeychainService, KeychainUser, error);
    if (error && error.type != keychain::ErrorType::NotFound) {
        printf("keychain error reading token: %s (%d)\n", error.message.c_str(), error.code);
    }

#else
    SMSTR("discord", "token", DiscordToken);
#endif

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
        SMBOOL("discord", "memory_db", UseMemoryDB);
        SMBOOL("discord", "prefetch", Prefetch);
        SMBOOL("discord", "autoconnect", Autoconnect);
        SMSTR("gui", "css", MainCSS);
        SMBOOL("gui", "animated_guild_hover_only", AnimatedGuildHoverOnly);
        SMBOOL("gui", "animations", ShowAnimations);
        SMBOOL("gui", "custom_emojis", ShowCustomEmojis);
        SMBOOL("gui", "member_list_discriminator", ShowMemberListDiscriminators);
        SMBOOL("gui", "owner_crown", ShowOwnerCrown);
        SMBOOL("gui", "save_state", SaveState);
        SMBOOL("gui", "stock_emojis", ShowStockEmojis);
        SMBOOL("gui", "unreads", Unreads);
        SMBOOL("gui", "alt_menu", AltMenu);
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

#ifdef WITH_KEYCHAIN
        keychain::Error error {};

        keychain::setPassword(KeychainPackage, KeychainService, KeychainUser, m_settings.DiscordToken, error);
        if (error) {
            printf("keychain error setting token: %s\n", error.message.c_str());
        }
#else
        SMSTR("discord", "token", DiscordToken);
#endif

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
