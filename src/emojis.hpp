#pragma once

#include <string>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include <gdkmm/pixbuf.h>
#include <gtkmm/textbuffer.h>

// shoutout to gtk for only supporting .svg's sometimes

class EmojiResource {
public:
    EmojiResource(std::string filepath);
    bool Load();
    Glib::RefPtr<Gdk::Pixbuf> GetPixBuf(const Glib::ustring &pattern);
    const std::map<std::string, std::string> &GetShortCodes() const;
    void ReplaceEmojis(Glib::RefPtr<Gtk::TextBuffer> buf, int size = 24);
    std::string GetShortCodeForPattern(const Glib::ustring &pattern);

private:
    std::unordered_map<std::string, std::vector<std::string>> m_pattern_shortcode_index;
    std::map<std::string, std::string> m_shortcode_index;         // shortcode -> pattern
    std::unordered_map<std::string, std::pair<int, int>> m_index; // pattern -> [pos, len]
    FILE *m_fp = nullptr;
    std::string m_filepath;
    std::vector<Glib::ustring> m_patterns;
};
