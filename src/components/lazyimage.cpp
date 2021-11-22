#include "lazyimage.hpp"
#include "abaddon.hpp"

LazyImage::LazyImage(int w, int h, bool use_placeholder)
    : m_width(w)
    , m_height(h) {
    if (use_placeholder)
        property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(w)->scale_simple(w, h, Gdk::INTERP_BILINEAR);
    signal_draw().connect(sigc::mem_fun(*this, &LazyImage::OnDraw));
}

LazyImage::LazyImage(const std::string &url, int w, int h, bool use_placeholder)
    : m_url(url)
    , m_width(w)
    , m_height(h) {
    if (use_placeholder)
        property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(w)->scale_simple(w, h, Gdk::INTERP_BILINEAR);
    signal_draw().connect(sigc::mem_fun(*this, &LazyImage::OnDraw));
}

void LazyImage::SetAnimated(bool is_animated) {
    m_animated = is_animated;
}

void LazyImage::SetURL(const std::string &url) {
    m_url = url;
}

bool LazyImage::OnDraw(const Cairo::RefPtr<Cairo::Context> &context) {
    if (!m_needs_request || m_url == "") return false;
    m_needs_request = false;

    if (m_animated) {
        auto cb = [this](const Glib::RefPtr<Gdk::PixbufAnimation> &pb) {
            property_pixbuf_animation() = pb;
        };

        Abaddon::Get().GetImageManager().LoadAnimationFromURL(m_url, m_width, m_height, sigc::track_obj(cb, *this));
    } else {
        auto cb = [this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            property_pixbuf() = pb->scale_simple(m_width, m_height, Gdk::INTERP_BILINEAR);
        };

        Abaddon::Get().GetImageManager().LoadFromURL(m_url, sigc::track_obj(cb, *this));
    }

    return false;
}
