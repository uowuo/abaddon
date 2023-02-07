#include "statusindicator.hpp"

static const constexpr int Diameter = 8;

StatusIndicator::StatusIndicator(Snowflake user_id)
    : Glib::ObjectBase("statusindicator")
    , Gtk::Widget()
    , m_id(user_id)
    , m_status(static_cast<PresenceStatus>(-1)) {
    set_has_window(true);
    set_name("status-indicator");

    get_style_context()->add_class("status-indicator");

    Abaddon::Get().GetDiscordClient().signal_guild_member_list_update().connect(sigc::hide(sigc::mem_fun(*this, &StatusIndicator::CheckStatus)));
    auto cb = [this](const UserData &user, PresenceStatus status) {
        if (user.ID == m_id) CheckStatus();
    };
    Abaddon::Get().GetDiscordClient().signal_presence_update().connect(sigc::track_obj(cb, *this));

    CheckStatus();
}

void StatusIndicator::CheckStatus() {
    const auto status = Abaddon::Get().GetDiscordClient().GetUserStatus(m_id);
    const auto last_status = m_status;
    get_style_context()->remove_class("online");
    get_style_context()->remove_class("dnd");
    get_style_context()->remove_class("idle");
    get_style_context()->remove_class("offline");
    get_style_context()->add_class(GetPresenceString(status));
    m_status = status;

    if (last_status != m_status)
        queue_draw();
}

Gtk::SizeRequestMode StatusIndicator::get_request_mode_vfunc() const {
    return Gtk::Widget::get_request_mode_vfunc();
}

void StatusIndicator::get_preferred_width_vfunc(int &minimum_width, int &natural_width) const {
    minimum_width = 0;
    natural_width = Diameter;
}

void StatusIndicator::get_preferred_height_for_width_vfunc(int width, int &minimum_height, int &natural_height) const {
    minimum_height = 0;
    natural_height = Diameter;
}

void StatusIndicator::get_preferred_height_vfunc(int &minimum_height, int &natural_height) const {
    minimum_height = 0;
    natural_height = Diameter;
}

void StatusIndicator::get_preferred_width_for_height_vfunc(int height, int &minimum_width, int &natural_width) const {
    minimum_width = 0;
    natural_width = Diameter;
}

void StatusIndicator::on_size_allocate(Gtk::Allocation &allocation) {
    set_allocation(allocation);

    if (m_window)
        m_window->move_resize(allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height());
}

void StatusIndicator::on_map() {
    Gtk::Widget::on_map();
}

void StatusIndicator::on_unmap() {
    Gtk::Widget::on_unmap();
}

void StatusIndicator::on_realize() {
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

void StatusIndicator::on_unrealize() {
    m_window.reset();

    Gtk::Widget::on_unrealize();
}

bool StatusIndicator::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
    const auto allocation = get_allocation();
    const auto width = allocation.get_width();
    const auto height = allocation.get_height();

    const auto color = get_style_context()->get_color(Gtk::STATE_FLAG_NORMAL);

    cr->set_source_rgb(color.get_red(), color.get_green(), color.get_blue());
    cr->arc(width / 2.0, height / 2.0, width / 3.0, 0.0, 2 * (4 * std::atan(1)));
    cr->close_path();
    cr->fill_preserve();
    cr->stroke();

    return true;
}
