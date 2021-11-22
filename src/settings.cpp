#include "settings.hpp"
#include <filesystem>
#include <fstream>

SettingsManager::SettingsManager(std::string filename)
    : m_filename(filename) {
    if (!std::filesystem::exists(filename)) {
        std::fstream fs;
        fs.open(filename, std::ios::out);
        fs.close();
    }

    auto rc = m_ini.LoadFile(filename.c_str());
    m_ok = rc == SI_OK;
}

void SettingsManager::Reload() {
    m_ok = m_ini.LoadFile(m_filename.c_str()) == SI_OK;
}

std::string SettingsManager::GetSettingString(const std::string &section, const std::string &key, std::string fallback) const {
    return m_ini.GetValue(section.c_str(), key.c_str(), fallback.c_str());
}

int SettingsManager::GetSettingInt(const std::string &section, const std::string &key, int fallback) const {
    return std::stoul(GetSettingString(section, key, std::to_string(fallback)));
}

bool SettingsManager::GetSettingBool(const std::string &section, const std::string &key, bool fallback) const {
    return GetSettingString(section, key, fallback ? "true" : "false") != "false";
}

bool SettingsManager::IsValid() const {
    return m_ok;
}

void SettingsManager::Close() {
    m_ini.SaveFile(m_filename.c_str());
}

bool SettingsManager::GetUseMemoryDB() const {
    return GetSettingBool("discord", "memory_db", false);
}

std::string SettingsManager::GetUserAgent() const {
    return GetSettingString("http", "user_agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36");
}

std::string SettingsManager::GetDiscordToken() const {
    return GetSettingString("discord", "token");
}

bool SettingsManager::GetShowMemberListDiscriminators() const {
    return GetSettingBool("gui", "member_list_discriminator", true);
}

bool SettingsManager::GetShowStockEmojis() const {
#ifdef _WIN32
    return GetSettingBool("gui", "stock_emojis", false);
#else
    return GetSettingBool("gui", "stock_emojis", true);
#endif
}

bool SettingsManager::GetShowCustomEmojis() const {
    return GetSettingBool("gui", "custom_emojis", true);
}

std::string SettingsManager::GetLinkColor() const {
    return GetSettingString("style", "linkcolor", "rgba(40, 200, 180, 255)");
}

std::string SettingsManager::GetChannelsExpanderColor() const {
    return GetSettingString("style", "expandercolor", "rgba(255, 83, 112, 255)");
}

std::string SettingsManager::GetNSFWChannelColor() const {
    return GetSettingString("style", "nsfwchannelcolor", "#ed6666");
}

int SettingsManager::GetCacheHTTPConcurrency() const {
    return GetSettingInt("http", "concurrent", 20);
}

bool SettingsManager::GetPrefetch() const {
    return GetSettingBool("discord", "prefetch", false);
}

std::string SettingsManager::GetMainCSS() const {
    return GetSettingString("gui", "css", "main.css");
}

bool SettingsManager::GetShowAnimations() const {
    return GetSettingBool("gui", "animations", true);
}

bool SettingsManager::GetShowOwnerCrown() const {
    return GetSettingBool("gui", "owner_crown", true);
}

std::string SettingsManager::GetGatewayURL() const {
    return GetSettingString("discord", "gateway", "wss://gateway.discord.gg/?v=9&encoding=json&compress=zlib-stream");
}

std::string SettingsManager::GetAPIBaseURL() const {
    return GetSettingString("discord", "api_base", "https://discord.com/api/v9");
}

bool SettingsManager::GetAnimatedGuildHoverOnly() const {
    return GetSettingBool("gui", "animated_guild_hover_only", true);
}

bool SettingsManager::GetSaveState() const {
    return GetSettingBool("gui", "save_state", true);
}
