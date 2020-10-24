#pragma once
#include <string>
#include <cstdio>
#include <unordered_map>
#include <vector>
#include <gtkmm.h>

// shoutout to gtk for only supporting .svg's sometimes

class EmojiResource {
public:
    EmojiResource(std::string filepath);
    bool Load();
    Glib::RefPtr<Gdk::Pixbuf> GetPixBuf(const Glib::ustring &pattern);
    static Glib::ustring PatternToHex(const Glib::ustring &pattern);
    static Glib::ustring HexToPattern(Glib::ustring hex);
    const std::vector<Glib::ustring> &GetPatterns() const;

private:
    std::unordered_map<std::string, std::pair<int, int>> m_index; // pattern -> [pos, len]
    FILE *m_fp = nullptr;
    std::string m_filepath;
    std::vector<Glib::ustring> m_patterns;
};
