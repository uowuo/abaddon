#include "emojis.hpp"

#include <sstream>
#include <utility>

#include <gdkmm/pixbufloader.h>

EmojiResource::EmojiResource(std::string filepath)
    : m_filepath(std::move(filepath)) {}

EmojiResource::~EmojiResource() {
    sqlite3_finalize(m_get_emoji_stmt);
    sqlite3_close(m_db);
}

bool EmojiResource::Load() {
    if (sqlite3_open(m_filepath.c_str(), &m_db) != SQLITE_OK) return false;

    if (sqlite3_prepare_v2(m_db, "SELECT data FROM emoji_data WHERE emoji = ?", -1, &m_get_emoji_stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(m_db, "SELECT * FROM emoji_shortcodes", -1, &stmt, nullptr) != SQLITE_OK) return false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string shortcode = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string emoji = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        m_shortcode_index[shortcode] = emoji;
        m_pattern_shortcode_index[emoji].push_back(shortcode);
    }
    sqlite3_finalize(stmt);

    std::sort(m_patterns.begin(), m_patterns.end(), [](const Glib::ustring &a, const Glib::ustring &b) {
        return a.size() > b.size();
    });

    return true;
}

Glib::RefPtr<Gdk::Pixbuf> EmojiResource::GetPixBuf(const Glib::ustring &pattern) {
    if (sqlite3_reset(m_get_emoji_stmt) != SQLITE_OK) return {};
    if (sqlite3_bind_text(m_get_emoji_stmt, 1, pattern.c_str(), -1, nullptr) != SQLITE_OK) return {};
    if (sqlite3_step(m_get_emoji_stmt) != SQLITE_ROW) return {};
    if (const void *blob = sqlite3_column_blob(m_get_emoji_stmt, 0)) {
        const int bytes = sqlite3_column_bytes(m_get_emoji_stmt, 0);
        auto loader = Gdk::PixbufLoader::create();
        loader->write(static_cast<const guint8 *>(blob), bytes);
        loader->close();
        return loader->get_pixbuf();
    }
    return {};
}

void EmojiResource::ReplaceEmojis(Glib::RefPtr<Gtk::TextBuffer> buf, int size) {
    auto get_text = [&]() -> auto {
        Gtk::TextBuffer::iterator a, b;
        buf->get_bounds(a, b);
        return buf->get_slice(a, b, true);
    };
    auto text = get_text();

    int searchpos;
    for (const auto &pattern : m_patterns) {
        searchpos = 0;
        Glib::RefPtr<Gdk::Pixbuf> pixbuf;
        while (true) {
            size_t r = text.find(pattern, searchpos);
            if (r == Glib::ustring::npos) break;
            if (!pixbuf) {
                pixbuf = GetPixBuf(pattern);
                if (pixbuf)
                    pixbuf = pixbuf->scale_simple(size, size, Gdk::INTERP_BILINEAR);
                else
                    break;
            }
            searchpos = static_cast<int>(r + pattern.size());

            const auto start_it = buf->get_iter_at_offset(static_cast<int>(r));
            const auto end_it = buf->get_iter_at_offset(static_cast<int>(r + pattern.size()));

            auto it = buf->erase(start_it, end_it);
            buf->insert_pixbuf(it, pixbuf);

            int alen = static_cast<int>(text.size());
            text = get_text();
            int blen = static_cast<int>(text.size());
            searchpos -= (alen - blen);
        }
    }
}

std::string EmojiResource::GetShortCodeForPattern(const Glib::ustring &pattern) {
    auto it = m_pattern_shortcode_index.find(pattern);
    if (it != m_pattern_shortcode_index.end())
        return it->second.front();
    return "";
}

const std::map<std::string, std::string> &EmojiResource::GetShortCodes() const {
    return m_shortcode_index;
}
