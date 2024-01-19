#include "cellrendererpixbufanimation.hpp"
#include <gdkmm/general.h>
#include <glibmm/main.h>

CellRendererPixbufAnimation::CellRendererPixbufAnimation()
    : Glib::ObjectBase(typeid(CellRendererPixbufAnimation))
    , Gtk::CellRenderer()
    , m_property_pixbuf(*this, "pixbuf")
    , m_property_pixbuf_animation(*this, "pixbuf-animation") {
    property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
    property_xpad() = 2;
    property_ypad() = 2;
}

Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf>> CellRendererPixbufAnimation::property_pixbuf() {
    return m_property_pixbuf.get_proxy();
}

Glib::PropertyProxy<Glib::RefPtr<Gdk::PixbufAnimation>> CellRendererPixbufAnimation::property_pixbuf_animation() {
    return m_property_pixbuf_animation.get_proxy();
}

void CellRendererPixbufAnimation::get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    int width = 0;

    if (auto pixbuf = m_property_pixbuf_animation.get_value())
        width = pixbuf->get_width();
    else if (auto pixbuf = m_property_pixbuf.get_value())
        width = pixbuf->get_width();

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_width = natural_width = xpad * 2 + width;
}

void CellRendererPixbufAnimation::get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc(widget, minimum_width, natural_width);
}

void CellRendererPixbufAnimation::get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    int height = 0;

    if (auto pixbuf = m_property_pixbuf_animation.get_value())
        height = pixbuf->get_height();
    else if (auto pixbuf = m_property_pixbuf.get_value())
        height = pixbuf->get_height();

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_height = natural_height = ypad * 2 + height;
}

void CellRendererPixbufAnimation::get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc(widget, minimum_height, natural_height);
}

void CellRendererPixbufAnimation::render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr,
                                               Gtk::Widget &widget,
                                               const Gdk::Rectangle &background_area,
                                               const Gdk::Rectangle &cell_area,
                                               Gtk::CellRendererState flags) {
    Gtk::Requisition minimum, natural;
    get_preferred_size(widget, minimum, natural);
    int xpad, ypad;
    get_padding(xpad, ypad);
    int pix_x = cell_area.get_x() + xpad;
    int pix_y = cell_area.get_y() + ypad;
    natural.width -= xpad * 2;
    natural.height -= ypad * 2;

    Gdk::Rectangle pix_rect(pix_x, pix_y, natural.width, natural.height);
    if (!cell_area.intersects(pix_rect))
        return;

    if (auto anim = m_property_pixbuf_animation.get_value()) {
        auto map_iter = m_pixbuf_animation_iters.find(anim);
        if (map_iter == m_pixbuf_animation_iters.end())
            m_pixbuf_animation_iters[anim] = anim->get_iter(nullptr);
        auto pb_iter = m_pixbuf_animation_iters.at(anim);

        const auto cb = [this, &widget, anim] {
            if (m_pixbuf_animation_iters.at(anim)->advance())
                widget.queue_draw();
        };
        Glib::signal_timeout().connect_once(sigc::track_obj(cb, widget), pb_iter->get_delay_time());
        Gdk::Cairo::set_source_pixbuf(cr, pb_iter->get_pixbuf(), pix_x, pix_y);
        cr->rectangle(pix_x, pix_y, natural.width, natural.height);
        cr->fill();
    } else if (auto pixbuf = m_property_pixbuf.get_value()) {
        Gdk::Cairo::set_source_pixbuf(cr, pixbuf, pix_x, pix_y);
        cr->rectangle(pix_x, pix_y, natural.width, natural.height);
        cr->fill();
    }
}
