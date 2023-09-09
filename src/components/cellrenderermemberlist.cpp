#include "cellrenderermemberlist.hpp"

CellRendererMemberList::CellRendererMemberList()
    : Glib::ObjectBase(typeid(CellRendererMemberList))
    , Gtk::CellRenderer()
    , m_property_type(*this, "render-type")
    , m_property_id(*this, "id")
    , m_property_name(*this, "name") {
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
    m_renderer_text.render(cr, widget, background_area, cell_area, flags);
}
