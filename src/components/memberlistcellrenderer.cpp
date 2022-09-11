#include "memberlistcellrenderer.hpp"
#include <gdkmm/general.h>

CellRendererMemberList::CellRendererMemberList()
    : Glib::ObjectBase(typeid(CellRendererMemberList))
    , m_property_type(*this, "render-type")
    , m_property_id(*this, "id")
    , m_property_markup(*this, "markup")
    , m_property_icon(*this, "pixbuf") {
    m_renderer_text.property_ellipsize() = Pango::ELLIPSIZE_END;
    m_property_markup.get_proxy().signal_changed().connect([this]() {
        m_renderer_text.property_markup() = m_property_markup;
    });
}

Glib::PropertyProxy<MemberListRenderType> CellRendererMemberList::property_type() {
    return m_property_type.get_proxy();
}

Glib::PropertyProxy<uint64_t> CellRendererMemberList::property_id() {
    return m_property_id.get_proxy();
}

Glib::PropertyProxy<Glib::ustring> CellRendererMemberList::property_markup() {
    return m_property_markup.get_proxy();
}

Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf>> CellRendererMemberList::property_icon() {
    return m_property_icon.get_proxy();
}

void CellRendererMemberList::get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Member:
            return get_preferred_width_vfunc_member(widget, minimum_width, natural_width);
        case MemberListRenderType::Role:
            return get_preferred_width_vfunc_role(widget, minimum_width, natural_width);
    }
}

void CellRendererMemberList::get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Member:
            return get_preferred_width_for_height_vfunc_member(widget, height, minimum_width, natural_width);
        case MemberListRenderType::Role:
            return get_preferred_width_for_height_vfunc_role(widget, height, minimum_width, natural_width);
    }
}

void CellRendererMemberList::get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Member:
            return get_preferred_height_vfunc_member(widget, minimum_height, natural_height);
        case MemberListRenderType::Role:
            return get_preferred_height_vfunc_role(widget, minimum_height, natural_height);
    }
}

void CellRendererMemberList::get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Member:
            return get_preferred_height_for_width_vfunc_member(widget, width, minimum_height, natural_height);
        case MemberListRenderType::Role:
            return get_preferred_height_for_width_vfunc_role(widget, width, minimum_height, natural_height);
    }
}

void CellRendererMemberList::render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Member:
            return render_vfunc_member(cr, widget, background_area, cell_area, flags);
        case MemberListRenderType::Role:
            return render_vfunc_role(cr, widget, background_area, cell_area, flags);
    }
}

void CellRendererMemberList::get_preferred_width_vfunc_role(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    int text_min, text_nat;
    m_renderer_text.get_preferred_width(widget, text_min, text_nat);
    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_width = text_min + xpad * 2;
    natural_width = text_nat + xpad * 2;
}

void CellRendererMemberList::get_preferred_width_for_height_vfunc_role(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc_role(widget, minimum_width, natural_width);
}

void CellRendererMemberList::get_preferred_height_vfunc_role(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    int text_min, text_nat;
    m_renderer_text.get_preferred_height(widget, text_min, text_nat);
    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_height = text_min + ypad * 2;
    natural_height = text_nat + ypad * 2;
}

void CellRendererMemberList::get_preferred_height_for_width_vfunc_role(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc_role(widget, minimum_height, natural_height);
}

void CellRendererMemberList::render_vfunc_role(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition text_min, text_nat, min, nat;
    m_renderer_text.get_preferred_size(widget, text_min, text_nat);
    get_preferred_size(widget, min, nat);

    int x = background_area.get_x() + 5;
    int y = background_area.get_y() + background_area.get_height() / 2.0 - text_nat.height / 2.0;
    int w = text_nat.width;
    int h = text_nat.height;

    Gdk::Rectangle text_cell_area(x, y, w, h);
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}

void CellRendererMemberList::get_preferred_width_vfunc_member(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    int text_min, text_nat;
    m_renderer_text.get_preferred_width(widget, text_min, text_nat);
    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_width = text_min + xpad * 2;
    natural_width = text_nat + xpad * 2;
}

void CellRendererMemberList::get_preferred_width_for_height_vfunc_member(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc_role(widget, minimum_width, natural_width);
}

void CellRendererMemberList::get_preferred_height_vfunc_member(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    int text_min, text_nat;
    m_renderer_text.get_preferred_height(widget, text_min, text_nat);
    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_height = text_min + ypad * 2;
    natural_height = text_nat + ypad * 2;
}

void CellRendererMemberList::get_preferred_height_for_width_vfunc_member(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc_role(widget, minimum_height, natural_height);
}

void CellRendererMemberList::render_vfunc_member(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition text_min, text_nat, min, nat;
    m_renderer_text.get_preferred_size(widget, text_min, text_nat);
    get_preferred_size(widget, min, nat);

    int pixbuf_w = 0, pixbuf_h = 0;
    if (auto pixbuf = m_property_icon.get_value()) {
        pixbuf_w = pixbuf->get_width();
        pixbuf_h = pixbuf->get_height();
    }

    const double icon_w = pixbuf_w;
    const double icon_h = pixbuf_h;
    const double icon_x = background_area.get_x() + 3.0;
    const double icon_y = background_area.get_y() + background_area.get_height() / 2.0 - icon_h / 2.0;

    const double x = icon_x + icon_w + 5;
    const double y = background_area.get_y() + background_area.get_height() / 2.0 - text_nat.height / 2.0;
    const double w = text_nat.width;
    const double h = text_nat.height;

    if (auto pixbuf = m_property_icon.get_value()) {
        Gdk::Cairo::set_source_pixbuf(cr, pixbuf, icon_x, icon_y);
        cr->rectangle(icon_x, icon_y, icon_w, icon_h);
        cr->fill();
    }

    Gdk::Rectangle text_cell_area(
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(w),
        static_cast<int>(h));

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}
