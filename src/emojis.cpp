#include "emojis.hpp"
#include <sstream>

EmojiResource::EmojiResource(std::string filepath)
    : m_filepath(filepath) {}

bool EmojiResource::Load() {
    m_fp = std::fopen(m_filepath.c_str(), "rb");
    if (m_fp == nullptr) return false;

    int index_offset;
    std::fread(&index_offset, 4, 1, m_fp);
    std::fseek(m_fp, index_offset, SEEK_SET);

    int emojis_count;
    std::fread(&emojis_count, 4, 1, m_fp);
    for (int i = 0; i < emojis_count; i++) {
        std::vector<std::string> shortcodes;

        int shortcodes_count;
        std::fread(&shortcodes_count, 4, 1, m_fp);
        for (int j = 0; j < shortcodes_count; j++) {
            int shortcode_length;
            std::fread(&shortcode_length, 4, 1, m_fp);
            std::string shortcode(shortcode_length, '\0');
            std::fread(shortcode.data(), shortcode_length, 1, m_fp);
            shortcodes.push_back(std::move(shortcode));
        }

        int surrogates_count;
        std::fread(&surrogates_count, 4, 1, m_fp);
        std::string surrogates(surrogates_count, '\0');
        std::fread(surrogates.data(), surrogates_count, 1, m_fp);
        m_patterns.push_back(surrogates);

        int data_size, data_offset;
        std::fread(&data_size, 4, 1, m_fp);
        std::fread(&data_offset, 4, 1, m_fp);
        m_index[surrogates] = { data_offset, data_size };

        for (const auto &shortcode : shortcodes)
            m_shortcode_index[shortcode] = surrogates;

        m_pattern_shortcode_index[surrogates] = std::move(shortcodes);
    }

    std::sort(m_patterns.begin(), m_patterns.end(), [](const Glib::ustring &a, const Glib::ustring &b) {
        return a.size() > b.size();
    });
    return true;
}

Glib::RefPtr<Gdk::Pixbuf> EmojiResource::GetPixBuf(const Glib::ustring &pattern) {
    const auto it = m_index.find(pattern);
    if (it == m_index.end()) return Glib::RefPtr<Gdk::Pixbuf>();
    const int pos = it->second.first;
    const int len = it->second.second;
    std::fseek(m_fp, pos, SEEK_SET);
    std::vector<uint8_t> data(len);
    std::fread(data.data(), len, 1, m_fp);
    auto loader = Gdk::PixbufLoader::create();
    loader->write(static_cast<const guint8 *>(data.data()), data.size());
    loader->close();
    return loader->get_pixbuf();
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
            searchpos = r + pattern.size();

            const auto start_it = buf->get_iter_at_offset(r);
            const auto end_it = buf->get_iter_at_offset(r + pattern.size());

            auto it = buf->erase(start_it, end_it);
            buf->insert_pixbuf(it, pixbuf);

            int alen = text.size();
            text = get_text();
            int blen = text.size();
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

const std::vector<Glib::ustring> &EmojiResource::GetPatterns() const {
    return m_patterns;
}

const std::map<std::string, std::string> &EmojiResource::GetShortCodes() const {
    return m_shortcode_index;
}
