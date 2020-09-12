#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <gtkmm.h>
#include "filecache.hpp"

class ImageManager {
public:
    Cache &GetCache();
    void LoadFromURL(std::string url, std::function<void(Glib::RefPtr<Gdk::Pixbuf>)> cb);
    Glib::RefPtr<Gdk::Pixbuf> GetFromURLIfCached(std::string url);
    Glib::RefPtr<Gdk::Pixbuf> GetPlaceholder(int size);

private:
    std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>> m_pixs;
    Cache m_cache;
};
