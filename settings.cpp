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

std::string SettingsManager::GetSetting(std::string section, std::string key, std::string fallback) {
    return m_ini.GetValue(section.c_str(), key.c_str(), fallback.c_str());
}

void SettingsManager::SetSetting(std::string section, std::string key, std::string value) {
    m_ini.SetValue(section.c_str(), key.c_str(), value.c_str());
}

bool SettingsManager::IsValid() const {
    return m_ok;
}

void SettingsManager::Close() {
    m_ini.SaveFile(m_filename.c_str());
}
