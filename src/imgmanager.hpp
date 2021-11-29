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

    using callback_anim_type = sigc::slot<void(Glib::RefPtr<Gdk::PixbufAnimation>)>;
    using callback_type = sigc::slot<void(Glib::RefPtr<Gdk::Pixbuf>)>;

    Cache &GetCache();
    void ClearCache();
    void LoadFromURL(std::string url, callback_type cb);
    // animations need dimensions before loading since there is no (easy) way to scale a PixbufAnimation
    void LoadAnimationFromURL(std::string url, int w, int h, callback_anim_type cb);
    void Prefetch(std::string url);
    Glib::RefPtr<Gdk::Pixbuf> GetFromURLIfCached(std::string url);
    Glib::RefPtr<Gdk::PixbufAnimation> GetAnimationFromURLIfCached(std::string url, int w, int h);
    Glib::RefPtr<Gdk::Pixbuf> GetPlaceholder(int size);

private:
    Glib::RefPtr<Gdk::Pixbuf> ReadFileToPixbuf(std::string path);
    Glib::RefPtr<Gdk::PixbufAnimation> ReadFileToPixbufAnimation(std::string path, int w, int h);

    mutable std::mutex m_load_mutex;
    void RunCallbacks();
    Glib::Dispatcher m_cb_dispatcher;
    mutable std::mutex m_cb_mutex;
    std::queue<std::function<void()>> m_cb_queue;

    std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>> m_pixs;
    Cache m_cache;
};
