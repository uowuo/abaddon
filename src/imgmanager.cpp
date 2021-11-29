#include "imgmanager.hpp"
#include "util.hpp"
#include "abaddon.hpp"

ImageManager::ImageManager() {
    m_cb_dispatcher.connect(sigc::mem_fun(*this, &ImageManager::RunCallbacks));
}

Cache &ImageManager::GetCache() {
    return m_cache;
}

void ImageManager::ClearCache() {
    m_cache.ClearCache();
}

Glib::RefPtr<Gdk::Pixbuf> ImageManager::ReadFileToPixbuf(std::string path) {
    const auto &data = ReadWholeFile(path);
    if (data.size() == 0) return Glib::RefPtr<Gdk::Pixbuf>(nullptr);
    auto loader = Gdk::PixbufLoader::create();
    loader->signal_size_prepared().connect([&loader](int w, int h) {
        int cw, ch;
        GetImageDimensions(w, h, cw, ch); // what could go wrong
        loader->set_size(cw, ch);
    });
    loader->write(static_cast<const guint8 *>(data.data()), data.size());
    loader->close();
    return loader->get_pixbuf();
}

Glib::RefPtr<Gdk::PixbufAnimation> ImageManager::ReadFileToPixbufAnimation(std::string path, int w, int h) {
    const auto &data = ReadWholeFile(path);
    if (data.size() == 0) return Glib::RefPtr<Gdk::PixbufAnimation>(nullptr);
    auto loader = Gdk::PixbufLoader::create();
    loader->signal_size_prepared().connect([&loader, w, h](int, int) {
        loader->set_size(w, h);
    });
    loader->write(static_cast<const guint8 *>(data.data()), data.size());
    loader->close();
    return loader->get_animation();
}

void ImageManager::LoadFromURL(std::string url, callback_type cb) {
    sigc::signal<void(Glib::RefPtr<Gdk::Pixbuf>)> signal;
    signal.connect(cb);
    m_cache.GetFileFromURL(url, [this, url, signal](std::string path) {
        try {
            auto buf = ReadFileToPixbuf(path);
            if (!buf)
                printf("%s (%s) is null\n", url.c_str(), path.c_str());
            else {
                m_cb_mutex.lock();
                m_cb_queue.push([signal, buf]() { signal.emit(buf); });
                m_cb_dispatcher.emit();
                m_cb_mutex.unlock();
            }
        } catch (const std::exception &e) {
            fprintf(stderr, "err loading pixbuf from %s: %s\n", path.c_str(), e.what());
        }
    });
}

void ImageManager::LoadAnimationFromURL(std::string url, int w, int h, callback_anim_type cb) {
    sigc::signal<void(Glib::RefPtr<Gdk::PixbufAnimation>)> signal;
    signal.connect(cb);
    m_cache.GetFileFromURL(url, [this, url, signal, w, h](std::string path) {
        try {
            auto buf = ReadFileToPixbufAnimation(path, w, h);
            if (!buf)
                printf("%s (%s) is null\n", url.c_str(), path.c_str());
            else {
                m_cb_mutex.lock();
                m_cb_queue.push([signal, buf]() { signal.emit(buf); });
                m_cb_dispatcher.emit();
                m_cb_mutex.unlock();
            }
        } catch (const std::exception &e) {
            fprintf(stderr, "err loading pixbuf animation from %s: %s\n", path.c_str(), e.what());
        }
    });
}

void ImageManager::Prefetch(std::string url) {
    m_cache.GetFileFromURL(url, [](const auto &) {});
}

void ImageManager::RunCallbacks() {
    m_cb_mutex.lock();
    m_cb_queue.front()();
    m_cb_queue.pop();
    m_cb_mutex.unlock();
}

Glib::RefPtr<Gdk::Pixbuf> ImageManager::GetFromURLIfCached(std::string url) {
    std::string path = m_cache.GetPathIfCached(url);
    if (path != "")
        return ReadFileToPixbuf(path);

    return Glib::RefPtr<Gdk::Pixbuf>(nullptr);
}

Glib::RefPtr<Gdk::PixbufAnimation> ImageManager::GetAnimationFromURLIfCached(std::string url, int w, int h) {
    std::string path = m_cache.GetPathIfCached(url);
    if (path != "")
        return ReadFileToPixbufAnimation(path, w, h);

    return Glib::RefPtr<Gdk::PixbufAnimation>(nullptr);
}

Glib::RefPtr<Gdk::Pixbuf> ImageManager::GetPlaceholder(int size) {
    std::string name = "/placeholder" + std::to_string(size);
    if (m_pixs.find(name) != m_pixs.end())
        return m_pixs.at(name);

    try {
        auto buf = Gdk::Pixbuf::create_from_file(Abaddon::Get().GetResPath() + "/decamarks.png", size, size);
        m_pixs[name] = buf;
        return buf;
    } catch (std::exception &e) {
        fprintf(stderr, "error loading placeholder\n");
        return Glib::RefPtr<Gdk::Pixbuf>(nullptr);
    }
}
