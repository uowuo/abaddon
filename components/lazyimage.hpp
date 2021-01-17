#pragma once
#include <gtkmm.h>

// loads an image only when the widget is drawn for the first time
class LazyImage : public Gtk::Image {
public:
    LazyImage(int w, int h, bool use_placeholder = true);
    LazyImage(const std::string &url, int w, int h, bool use_placeholder = true);

    void SetURL(const std::string &url);

private:
    bool OnDraw(const Cairo::RefPtr<Cairo::Context> &context);

    bool m_needs_request = true;
    int m_idx;
    std::string m_url;
    int m_width;
    int m_height;
};
