#pragma once

#include <string>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include <gdkmm/pixbuf.h>
#include <gtkmm/textbuffer.h>

#include <sqlite3.h>

class EmojiResource {
public:
    EmojiResource(std::string filepath);
    ~EmojiResource();

    bool Load();
    Glib::RefPtr<Gdk::Pixbuf> GetPixBuf(const Glib::ustring &pattern);
    const std::map<std::string, std::string> &GetShortCodes() const;
    void ReplaceEmojis(Glib::RefPtr<Gtk::TextBuffer> buf, int size = 24);
    std::string GetShortCodeForPattern(const Glib::ustring &pattern);

private:
    std::unordered_map<std::string, std::vector<std::string>> m_pattern_shortcode_index;
    std::map<std::string, std::string> m_shortcode_index; // shortcode -> pattern
    std::string m_filepath;
    std::vector<Glib::ustring> m_patterns;

    sqlite3 *m_db = nullptr;
    sqlite3_stmt *m_get_emoji_stmt = nullptr;
};
