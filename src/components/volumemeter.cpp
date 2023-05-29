#include "volumemeter.hpp"
#include <cstring>

VolumeMeter::VolumeMeter()
    : Glib::ObjectBase("volumemeter")
    , Gtk::Widget() {
    set_has_window(true);
}

void VolumeMeter::SetVolume(double fraction) {
    m_fraction = fraction;
    queue_draw();
}

void VolumeMeter::SetTick(double fraction) {
    m_tick = fraction;
    queue_draw();
}

void VolumeMeter::SetShowTick(bool show) {
    m_show_tick = show;
}

Gtk::SizeRequestMode VolumeMeter::get_request_mode_vfunc() const {
    return Gtk::Widget::get_request_mode_vfunc();
}

void VolumeMeter::get_preferred_width_vfunc(int &minimum_width, int &natural_width) const {
    const int width = get_allocated_width();
    minimum_width = natural_width = width;
}

void VolumeMeter::get_preferred_width_for_height_vfunc(int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc(minimum_width, natural_width);
}

void VolumeMeter::get_preferred_height_vfunc(int &minimum_height, int &natural_height) const {
    // blehhh :PPP
    const int height = get_allocated_height();
    minimum_height = natural_height = 4;
}

void VolumeMeter::get_preferred_height_for_width_vfunc(int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc(minimum_height, natural_height);
}

void VolumeMeter::on_size_allocate(Gtk::Allocation &allocation) {
    set_allocation(allocation);

    if (m_window)
        m_window->move_resize(allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height());
}

void VolumeMeter::on_map() {
    Gtk::Widget::on_map();
}

void VolumeMeter::on_unmap() {
    Gtk::Widget::on_unmap();
}

void VolumeMeter::on_realize() {
    set_realized(true);

    if (!m_window) {
        GdkWindowAttr attributes;
        std::memset(&attributes, 0, sizeof(attributes));

        auto allocation = get_allocation();

        attributes.x = allocation.get_x();
        attributes.y = allocation.get_y();
        attributes.width = allocation.get_width();
        attributes.height = allocation.get_height();

        attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK;
        attributes.window_type = GDK_WINDOW_CHILD;
        attributes.wclass = GDK_INPUT_OUTPUT;

        m_window = Gdk::Window::create(get_parent_window(), &attributes, GDK_WA_X | GDK_WA_Y);
        set_window(m_window);

        m_window->set_user_data(gobj());
    }
}

void VolumeMeter::on_unrealize() {
    m_window.reset();

    Gtk::Widget::on_unrealize();
}

bool VolumeMeter::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
    const auto allocation = get_allocation();
    const auto width = allocation.get_width();
    const auto height = allocation.get_height();

    const double LOW_MAX = 0.7 * width;
    const double MID_MAX = 0.85 * width;
    const double desired_width = width * m_fraction;

    const double draw_low = std::min(desired_width, LOW_MAX);
    const double draw_mid = std::min(desired_width, MID_MAX);
    const double draw_hi = desired_width;

    cr->set_source_rgb(1.0, 0.0, 0.0);
    cr->rectangle(0.0, 0.0, draw_hi, height);
    cr->fill();
    cr->set_source_rgb(1.0, 0.5, 0.0);
    cr->rectangle(0.0, 0.0, draw_mid, height);
    cr->fill();
    cr->set_source_rgb(.0, 1.0, 0.0);
    cr->rectangle(0.0, 0.0, draw_low, height);
    cr->fill();

    if (m_show_tick) {
        const double tick_base = width * m_tick;

        cr->set_source_rgb(0.8, 0.8, 0.8);
        cr->rectangle(tick_base, 0, 4, height);
        cr->fill();
    }

    return true;
}
