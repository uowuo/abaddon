#include "platform.hpp"
#include <string>
#include <fstream>
#include <filesystem>

bool IsFolder(std::string_view path) {
    std::error_code ec;
    const auto status = std::filesystem::status(path, ec);
    if (ec) return false;
    return status.type() == std::filesystem::file_type::directory;
}

#if defined(_WIN32) && defined(_MSC_VER)
    #include <Windows.h>
    #include <Shlwapi.h>
    #include <ShlObj_core.h>
    #include <pango/pangocairo.h>
    #include <pango/pangofc-fontmap.h>
    #pragma comment(lib, "Shlwapi.lib")
bool Platform::SetupFonts() {
    using namespace std::string_literals;

    char buf[MAX_PATH] { 0 };
    GetCurrentDirectoryA(MAX_PATH, buf);
    {
        // thanks @WorkingRobot for da help :^))

        std::ifstream template_stream(buf + "\\fonts\\fonts.template.conf"s);
        std::ofstream conf_stream(buf + "\\fonts\\fonts.conf"s);
        if (!template_stream.good()) {
            printf("can't open fonts/fonts.template.conf\n");
            return false;
        }
        if (!conf_stream.good()) {
            printf("can't open write to fonts.conf\n");
            return false;
        }

        std::string line;
        while (std::getline(template_stream, line)) {
            if (line == "<!--(CONFD)-->")
                conf_stream << "<include ignore_missing=\"no\">" << (buf + "\\fonts\\conf.d"s) << "</include>";
            else
                conf_stream << line;
            conf_stream << '\n';
        }
    }

    auto fc = FcConfigCreate();
    FcConfigSetCurrent(fc);
    FcConfigParseAndLoad(fc, const_cast<FcChar8 *>(reinterpret_cast<const FcChar8 *>((buf + "\\fonts\\fonts.conf"s).c_str())), true);
    FcConfigAppFontAddDir(fc, const_cast<FcChar8 *>(reinterpret_cast<const FcChar8 *>((buf + "\\fonts"s).c_str())));

    char fonts_path[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_FONTS, NULL, SHGFP_TYPE_CURRENT, fonts_path) == S_OK) {
        FcConfigAppFontAddDir(fc, reinterpret_cast<FcChar8 *>(fonts_path));
    }

    auto map = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
    pango_fc_font_map_set_config(reinterpret_cast<PangoFcFontMap *>(map), fc);
    pango_cairo_font_map_set_default(reinterpret_cast<PangoCairoFontMap *>(map));

    return true;
}
#else
bool Platform::SetupFonts() {
    return true;
}
#endif

#if defined(_WIN32)
std::string Platform::FindResourceFolder() {
    return ".";
}

#elif defined(__linux__)
std::string Platform::FindResourceFolder() {
    static std::string path;
    static bool found = false;
    if (found) return path;

    if (IsFolder("/usr/share/abaddon/res") && IsFolder("/usr/share/abaddon/css")) {
        path = "/usr/share/abaddon";
    } else {
        puts("resources are not in /usr/share/abaddon, will try to load from cwd");
        path = ".";
    }
    found = true;
    return path;
}
#else
std::string Platform::FindResourceFolder() {
    puts("unknown OS, trying to load resources from cwd");
}
#endif
