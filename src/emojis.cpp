#include "emojis.hpp"
#include <sstream>
#include <utility>

#include <stdio.h>

/* Pre-processor hack for byte order conversions */
#if defined __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
/* this might not work on BSD's... only tested on Linux with GCC/glibc on
   powerpc 32-bit (Powerbook G4). */
#include <byteswap.h>
int emojis_bin_proc_int(int input) {
    return (int)__bswap_32((uint32_t)input);
}
#else
int emojis_bin_proc_int(int input) {
  return input;
}
#endif /* __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ */
#else /* no modern windows uses big endian... hope this is enough. */
int emojis_bin_proc_int(int input) {
  return input;
}
#endif /* defined __BYTE_ORDER__ */

EmojiResource::EmojiResource(std::string filepath)
    : m_filepath(std::move(filepath)) {}

bool EmojiResource::Load() {
    m_fp = std::fopen(m_filepath.c_str(), "rb");
    if (m_fp == nullptr) return false;

    int index_offset;
    std::fread(&index_offset, 4, 1, m_fp);
    index_offset=emojis_bin_proc_int(index_offset);
    std::fseek(m_fp, index_offset, SEEK_SET);

    int emojis_count;
    std::fread(&emojis_count, 4, 1, m_fp);
    emojis_count=emojis_bin_proc_int(emojis_count);
    for (int i = 0; i < emojis_count; i++) {
        std::vector<std::string> shortcodes;

        int shortcodes_count;
        std::fread(&shortcodes_count, 4, 1, m_fp);
        shortcodes_count=emojis_bin_proc_int(shortcodes_count);
        for (int j = 0; j < shortcodes_count; j++) {
            int shortcode_length;
            std::fread(&shortcode_length, 4, 1, m_fp);
            shortcode_length=emojis_bin_proc_int(shortcode_length);
            std::string shortcode(shortcode_length, '\0');
            std::fread(shortcode.data(), shortcode_length, 1, m_fp);
            shortcodes.push_back(std::move(shortcode));
        }

        int surrogates_count;
        std::fread(&surrogates_count, 4, 1, m_fp);
        surrogates_count=emojis_bin_proc_int(surrogates_count);
        std::string surrogates(surrogates_count, '\0');
        std::fread(surrogates.data(), surrogates_count, 1, m_fp);
        m_patterns.emplace_back(surrogates);

        int data_size, data_offset;
        std::fread(&data_size, 4, 1, m_fp);
        data_size=emojis_bin_proc_int(data_size);
        std::fread(&data_offset, 4, 1, m_fp);
        data_offset=emojis_bin_proc_int(data_offset);
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
    if (it == m_index.end()) return {};
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
