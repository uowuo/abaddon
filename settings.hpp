#pragma once
#include <string>
#include <type_traits>
#include <SimpleIni.h>

class SettingsManager {
public:
    SettingsManager(std::string filename);

    void Close();
    std::string GetSettingString(std::string section, std::string key, std::string fallback = "") const;
    int GetSettingInt(std::string section, std::string key, int fallback) const;

    template<typename T>
    void SetSetting(std::string section, std::string key, T value) {
        static_assert(std::is_convertible<T, std::string>::value);
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
