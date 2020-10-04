#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <queue>
#include <gtkmm.h>
#include "filecache.hpp"

class ImageManager {
public:
    ImageManager();

    using callback_type = sigc::slot<void(Glib::RefPtr<Gdk::Pixbuf>)>;

    Cache &GetCache();
    void LoadFromURL(std::string url, callback_type cb);
    Glib::RefPtr<Gdk::Pixbuf> GetFromURLIfCached(std::string url);
    Glib::RefPtr<Gdk::Pixbuf> GetPlaceholder(int size);

private:
    Glib::RefPtr<Gdk::Pixbuf> ReadFileToPixbuf(std::string path);

    mutable std::mutex m_load_mutex;
    void RunCallbacks();
    Glib::Dispatcher m_cb_dispatcher;
    mutable std::mutex m_cb_mutex;
    std::queue<std::function<void()>> m_cb_queue;

    std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>> m_pixs;
    Cache m_cache;
};
