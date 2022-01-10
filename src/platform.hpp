#pragma once
#include <string>

namespace Platform {
bool SetupFonts();
std::string FindResourceFolder();
std::string FindConfigFile();
std::string FindStateCacheFolder();
} // namespace Platform
