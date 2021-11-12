#include "settings.hpp"
#include <filesystem>
#include <fstream>

SettingsManager::SettingsManager(std::string_view filename)
    : m_filename(filename)
    , m_ok(false) {
    if (!std::filesystem::exists(m_filename)) {
        std::fstream fs;
        fs.open(m_filename, std::ios::out);
        fs.close();
    }

    Reload();
}

void SettingsManager::Reload() {
    m_ok = LoadFile();
}

SettingsManager::json_type const *SettingsManager::GetValue(std::string_view section, std::string_view key) const noexcept {
    if (!m_json.is_object())
        return nullptr;
    if (auto it = m_json.find(section.data()); it != m_json.end())
        if (auto it2 = it->find(key.data()); it2 != it->end())
            return &*it2;
    return nullptr;
}

std::string SettingsManager::GetSettingString(std::string_view section, std::string_view key, std::string_view fallback) const {
    auto const *v = GetValue(section, key);
    if (v != nullptr && v->is_string())
        return v->get<std::string>();
    return std::string { fallback };
}

int SettingsManager::GetSettingInt(std::string_view section, std::string_view key, int fallback) const {
    auto const *v = GetValue(section, key);
    if (v != nullptr && v->is_number_integer())
        return v->get<int>();
    return fallback;
}

bool SettingsManager::GetSettingBool(std::string_view section, std::string_view key, bool fallback) const {
    auto const *v = GetValue(section, key);
    if (v != nullptr && v->is_boolean())
        return v->get<bool>();
    return fallback;
}

bool SettingsManager::LoadFile() noexcept {
    try {
        std::ifstream(m_filename, std::ofstream::in) >> m_json;
    } catch (const std::exception &e) {
        std::printf("Error while reading config file %s: %s\n", m_filename.data(), e.what());
        return false;
    }
    return true;
}

void SettingsManager::SaveFile() {
    std::ofstream f(m_filename, std::ofstream::out);
    f << std::setw(1) << std::setfill('\t') << m_json;
}

bool SettingsManager::IsValid() const {
    return m_ok;
}

void SettingsManager::Close() {
    SaveFile(); // Maybe just make SaveFile public?
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
