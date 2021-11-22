#pragma once
#include <string>
#include <type_traits>
#include <SimpleIni.h>

class SettingsManager {
public:
    SettingsManager(std::string filename);
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

    template<typename T>
    void SetSetting(std::string section, std::string key, T value) {
        m_ini.SetValue(section.c_str(), key.c_str(), std::to_string(value).c_str());
        m_ini.SaveFile(m_filename.c_str());
    }

    void SetSetting(std::string section, std::string key, std::string value) {
        m_ini.SetValue(section.c_str(), key.c_str(), value.c_str());
        m_ini.SaveFile(m_filename.c_str());
    }

private:
    std::string GetSettingString(const std::string &section, const std::string &key, std::string fallback = "") const;
    int GetSettingInt(const std::string &section, const std::string &key, int fallback) const;
    bool GetSettingBool(const std::string &section, const std::string &key, bool fallback) const;

private:
    bool m_ok;
    std::string m_filename;
    CSimpleIniA m_ini;
};
