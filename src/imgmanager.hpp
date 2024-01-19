#pragma once

#include <functional>
#include <queue>
#include <string>
#include <unordered_map>

#include <gdkmm/pixbuf.h>
#include <gdkmm/pixbufanimation.h>
#include <glibmm/dispatcher.h>

#include "filecache.hpp"

class ImageManager {
public:
    ImageManager();

    using callback_anim_type = sigc::slot<void(Glib::RefPtr<Gdk::PixbufAnimation>)>;
    using callback_type = sigc::slot<void(Glib::RefPtr<Gdk::Pixbuf>)>;

    void ClearCache();
    void LoadFromURL(const std::string &url, const callback_type &cb);
    // animations need dimensions before loading since there is no (easy) way to scale a PixbufAnimation
    void LoadAnimationFromURL(const std::string &url, int w, int h, const callback_anim_type &cb);
    void Prefetch(const std::string &url);
    Glib::RefPtr<Gdk::Pixbuf> GetPlaceholder(int size);
    Cache &GetCache();

private:
    static Glib::RefPtr<Gdk::Pixbuf> ReadFileToPixbuf(std::string path);
    static Glib::RefPtr<Gdk::PixbufAnimation> ReadFileToPixbufAnimation(std::string path, int w, int h);

    mutable std::mutex m_load_mutex;
    void RunCallbacks();
    Glib::Dispatcher m_cb_dispatcher;
    mutable std::mutex m_cb_mutex;
    std::queue<std::function<void()>> m_cb_queue;

    std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>> m_pixs;
    Cache m_cache;
};
