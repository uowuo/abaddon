#pragma once
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

class SettingsManager {
public:
    SettingsManager(std::string_view filename);
    void Reload();

    void Close();
    bool GetUseMemoryDB() const;
    std::string GetUserAgent() const;
    std::string GetDiscordToken() const;
    bool GetShowMemberListDiscriminators() const;
    bool GetShowStockEmojis() const;
    bool GetShowCustomEmojis() const;
    int GetCacheHTTPConcurrency() const;
    bool GetPrefetch() const;
    std::string GetMainCSS() const;
    bool GetShowAnimations() const;
    bool GetShowOwnerCrown() const;
    std::string GetGatewayURL() const;
    std::string GetAPIBaseURL() const;
    bool GetAnimatedGuildHoverOnly() const;
    bool GetSaveState() const;

    // i would like to use Gtk::StyleProperty for this, but it will not work on windows
    // #1 it's missing from the project files for the version used by vcpkg
    // #2 it's still broken and doesn't function even when added to the solution
    // #3 it's a massive pain in the ass to try and bump the version to a functioning version
    //    because they switch build systems to nmake/meson (took months to get merged in vcpkg)
    // #4 c++ build systems sucks
    // three options are: use gtk4 with updated vcpkg, try and port it myself, or use msys2 instead of vcpkg
    // im leaning towards msys
    std::string GetLinkColor() const;
    std::string GetChannelsExpanderColor() const;
    std::string GetNSFWChannelColor() const;

    bool IsValid() const;

    void SetSetting(std::string_view section, std::string_view key, std::string_view value) {
        m_json[section.data()][key.data()] = value.data();
        SaveFile();
    }

    void SetSetting(std::string_view section, std::string_view key, const std::string &value) {
        SetSetting(section, key, std::string_view { value });
    }

    template<typename T>
    void SetSetting(std::string_view section, std::string_view key, T value) {
        m_json[section.data()][key.data()] = value;
        SaveFile();
    }

private:
    using json_type = nlohmann::ordered_json;

    json_type const *GetValue(std::string_view section, std::string_view key) const noexcept;
    std::string GetSettingString(std::string_view section, std::string_view key, std::string_view fallback = "") const;
    int GetSettingInt(std::string_view section, std::string_view key, int fallback) const;
    bool GetSettingBool(std::string_view section, std::string_view key, bool fallback) const;

    bool LoadFile() noexcept;
    void SaveFile();

private:
    const std::string m_filename;
    bool m_ok;
    json_type m_json;
};
