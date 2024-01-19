#pragma once
#include <gtkmm/image.h>

// loads an image only when the widget is drawn for the first time
class LazyImage : public Gtk::Image {
public:
    LazyImage(int w, int h, bool use_placeholder = true);
    LazyImage(std::string url, int w, int h, bool use_placeholder = true);

    void SetAnimated(bool is_animated);
    void SetURL(const std::string &url);

private:
    bool OnDraw(const Cairo::RefPtr<Cairo::Context> &context);

    bool m_animated = false;
    bool m_needs_request = true;
    std::string m_url;
    int m_width;
    int m_height;
};
