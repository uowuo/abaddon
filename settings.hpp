#pragma once
#include <string>
#include <SimpleIni.h>

class SettingsManager {
public:
    SettingsManager(std::string filename);

    void Close();
    std::string GetSetting(std::string section, std::string key, std::string fallback = "");
    void SetSetting(std::string section, std::string key, std::string value);
    bool IsValid() const;

private:
    bool m_ok;
    std::string m_filename;
    CSimpleIniA m_ini;
};
