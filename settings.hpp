#pragma once
#include <string>
#include <type_traits>
#include <SimpleIni.h>

class SettingsManager {
public:
    SettingsManager(std::string filename);

    void Close();
    std::string GetSettingString(const std::string &section, const std::string &key, std::string fallback = "") const;
    int GetSettingInt(const std::string &section, const std::string &key, int fallback) const;
    bool GetSettingBool(const std::string &section, const std::string &key, bool fallback) const;

    template<typename T>
    void SetSetting(std::string section, std::string key, T value) {
        if constexpr (std::is_same<T, std::string>::value)
            m_ini.SetValue(section.c_str(), key.c_str(), value.c_str());
        else
            m_ini.SetValue(section.c_str(), key.c_str(), std::to_string(value).c_str());

        m_ini.SaveFile(m_filename.c_str());
    }

    bool IsValid() const;

private:
    bool m_ok;
    std::string m_filename;
    CSimpleIniA m_ini;
};
