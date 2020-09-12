#include "imgmanager.hpp"

Cache &ImageManager::GetCache() {
    return m_cache;
}

void ImageManager::LoadFromURL(std::string url, std::function<void(Glib::RefPtr<Gdk::Pixbuf>)> cb) {
    if (m_pixs.find(url) != m_pixs.end()) {
        cb(m_pixs.at(url));
        return;
    }

    m_cache.GetFileFromURL(url, [this, url, cb](std::string path) {
        try {
            auto buf = Gdk::Pixbuf::create_from_file(path);
            m_pixs[url] = buf;
            cb(buf);
        } catch (std::exception &e) {
            fprintf(stderr, "err loading pixbuf from %s: %s\n", path.c_str(), e.what());
        }
    });
}

Glib::RefPtr<Gdk::Pixbuf> ImageManager::GetFromURLIfCached(std::string url) {
    if (m_pixs.find(url) != m_pixs.end())
        return m_pixs.at(url);

    return Glib::RefPtr<Gdk::Pixbuf>(nullptr);
}
