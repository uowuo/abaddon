#include "cellrenderermemberlist.hpp"
#include <gdkmm/general.h>

CellRendererMemberList::CellRendererMemberList()
    : Glib::ObjectBase(typeid(CellRendererMemberList))
    , m_property_type(*this, "render-type")
    , m_property_id(*this, "id")
    , m_property_name(*this, "name")
    , m_property_pixbuf(*this, "pixbuf")
    , m_property_color(*this, "color")
    , m_property_status(*this, "status") {
    property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
    property_xpad() = 2;
    property_ypad() = 2;
    m_property_name.get_proxy().signal_changed().connect([this]() {
        m_renderer_text.property_markup() = m_property_name;
    });
}

Glib::PropertyProxy<MemberListRenderType> CellRendererMemberList::property_type() {
    return m_property_type.get_proxy();
}

Glib::PropertyProxy<uint64_t> CellRendererMemberList::property_id() {
    return m_property_id.get_proxy();
}

Glib::PropertyProxy<Glib::ustring> CellRendererMemberList::property_name() {
    return m_property_name.get_proxy();
}

Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf>> CellRendererMemberList::property_pixbuf() {
    return m_property_pixbuf.get_proxy();
}

Glib::PropertyProxy<Gdk::RGBA> CellRendererMemberList::property_color() {
    return m_property_color.get_proxy();
}

Glib::PropertyProxy<PresenceStatus> CellRendererMemberList::property_status() {
    return m_property_status.get_proxy();
}

void CellRendererMemberList::get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Role:
            return get_preferred_width_vfunc_role(widget, minimum_width, natural_width);
        case MemberListRenderType::Member:
            return get_preferred_width_vfunc_member(widget, minimum_width, natural_width);
    }
}

void CellRendererMemberList::get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Role:
            return get_preferred_width_for_height_vfunc_role(widget, height, minimum_width, natural_width);
        case MemberListRenderType::Member:
            return get_preferred_width_for_height_vfunc_member(widget, height, minimum_width, natural_width);
    }
}

void CellRendererMemberList::get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Role:
            return get_preferred_height_vfunc_role(widget, minimum_width, natural_width);
        case MemberListRenderType::Member:
            return get_preferred_height_vfunc_member(widget, minimum_width, natural_width);
    }
}

void CellRendererMemberList::get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Role:
            return get_preferred_height_for_width_vfunc_role(widget, width, minimum_height, natural_height);
        case MemberListRenderType::Member:
            return get_preferred_height_for_width_vfunc_member(widget, width, minimum_height, natural_height);
    }
}

void CellRendererMemberList::render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    switch (m_property_type.get_value()) {
        case MemberListRenderType::Role:
            return render_vfunc_role(cr, widget, background_area, cell_area, flags);
        case MemberListRenderType::Member:
            return render_vfunc_member(cr, widget, background_area, cell_area, flags);
    }
}

void CellRendererMemberList::get_preferred_width_vfunc_role(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererMemberList::get_preferred_width_for_height_vfunc_role(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererMemberList::get_preferred_height_vfunc_role(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererMemberList::get_preferred_height_for_width_vfunc_role(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererMemberList::render_vfunc_role(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    m_renderer_text.render(cr, widget, background_area, cell_area, flags);
}

void CellRendererMemberList::get_preferred_width_vfunc_member(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererMemberList::get_preferred_width_for_height_vfunc_member(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererMemberList::get_preferred_height_vfunc_member(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererMemberList::get_preferred_height_for_width_vfunc_member(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererMemberList::render_vfunc_member(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    // Text
    Gdk::Rectangle text_cell_area = cell_area;
    text_cell_area.set_x(31);
    const auto color = m_property_color.get_value();
    if (color.get_alpha_u() > 0) {
        m_renderer_text.property_foreground_rgba().set_value(color);
    }
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
    m_renderer_text.property_foreground_set().set_value(false);

    // Status indicator
    // TODO: reintroduce custom status colors... somehow
    cr->begin_new_path();
    switch (m_property_status.get_value()) {
        case PresenceStatus::Online:
            cr->set_source_rgb(33.0 / 255.0, 157.0 / 255.0, 86.0 / 255.0);
            break;
        case PresenceStatus::Idle:
            cr->set_source_rgb(230.0 / 255.0, 170.0 / 255.0, 48.0 / 255.0);
            break;
        case PresenceStatus::DND:
            cr->set_source_rgb(233.0 / 255.0, 61.0 / 255.0, 65.0 / 255.0);
            break;
        case PresenceStatus::Offline:
            cr->set_source_rgb(122.0 / 255.0, 126.0 / 255.0, 135.0 / 255.0);
            break;
    }

    cr->arc(background_area.get_x() + 6.0 + 16.0 + 6.0, background_area.get_y() + background_area.get_height() / 2.0, 2.0, 0.0, 2 * (4 * std::atan(1)));
    cr->close_path();
    cr->fill_preserve();
    cr->stroke();

    // Icon
    const double icon_x = background_area.get_x() + 6.0;
    const double icon_y = background_area.get_y() + background_area.get_height() / 2.0 - 8.0;
    Gdk::Cairo::set_source_pixbuf(cr, m_property_pixbuf.get_value(), icon_x, icon_y);
    cr->rectangle(icon_x, icon_y, 16.0, 16.0);
    cr->fill();

    m_signal_render.emit(m_property_id.get_value());
}

CellRendererMemberList::type_signal_render CellRendererMemberList::signal_render() {
    return m_signal_render;
}
