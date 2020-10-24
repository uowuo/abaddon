#include "emojis.hpp"
#include <sstream>

EmojiResource::EmojiResource(std::string filepath)
    : m_filepath(filepath) {}

bool EmojiResource::Load() {
    m_fp = std::fopen(m_filepath.c_str(), "rb");
    if (m_fp == nullptr) return false;
    int index_pos;
    std::fread(&index_pos, 4, 1, m_fp);
    std::fseek(m_fp, index_pos, SEEK_SET);
    int num_entries;
    std::fread(&num_entries, 4, 1, m_fp);
    for (int i = 0; i < num_entries; i++) {
        static int strsize, len, pos;
        std::fread(&strsize, 4, 1, m_fp);
        std::vector<char> str(strsize);
        std::fread(str.data(), strsize, 1, m_fp);
        std::fread(&len, 4, 1, m_fp);
        std::fread(&pos, 4, 1, m_fp);
        m_index[std::string(str.begin(), str.end())] = std::make_pair(pos, len);
        m_patterns.push_back(HexToPattern(Glib::ustring(str.begin(), str.end())));
    }
    std::sort(m_patterns.begin(), m_patterns.end(), [](const Glib::ustring &a, const Glib::ustring &b) {
        return a.size() > b.size();
    });
    return true;
}

Glib::RefPtr<Gdk::Pixbuf> EmojiResource::GetPixBuf(const Glib::ustring &pattern) {
    Glib::ustring key = PatternToHex(pattern);
    const auto it = m_index.find(key);
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

Glib::ustring EmojiResource::PatternToHex(const Glib::ustring &pattern) {
    Glib::ustring ret;
    for (int i = 0; i < pattern.size(); i++) {
        auto c = pattern.at(i);
        ret += Glib::ustring::format(std::hex, c);
        ret += "-";
    }
    return ret.substr(0, ret.size() - 1);
}

Glib::ustring EmojiResource::HexToPattern(Glib::ustring hex) {
    std::vector<Glib::ustring> parts;
    Glib::ustring::size_type pos = 0;
    Glib::ustring token;
    while ((pos = hex.find("-")) != Glib::ustring::npos) {
        token = hex.substr(0, pos);
        if (token.length() % 2 == 1)
            token = "0" + token;
        parts.push_back(token);
        hex.erase(0, pos + 1);
    }
    parts.push_back(hex);
    Glib::ustring ret;
    for (const auto &part : parts) {
        auto x = static_cast<gunichar>(std::stoul(part.c_str(), nullptr, 16));
        ret += x;
    }
    return ret;
}

const std::vector<Glib::ustring> &EmojiResource::GetPatterns() const {
    return m_patterns;
}
