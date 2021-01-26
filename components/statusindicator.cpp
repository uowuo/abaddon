#include "statusindicator.hpp"
#include "../abaddon.hpp"

static const constexpr int Diameter = 8;
static const auto OnlineColor = Gdk::RGBA("#43B581");
static const auto IdleColor = Gdk::RGBA("#FAA61A");
static const auto DNDColor = Gdk::RGBA("#982929");
static const auto OfflineColor = Gdk::RGBA("#808080");

StatusIndicator::StatusIndicator(Snowflake user_id)
    : Glib::ObjectBase("statusindicator")
    , Gtk::Widget()
    , m_id(user_id)
    , m_color(OfflineColor) {
    set_has_window(true);
    set_name("status-indicator");

    Abaddon::Get().GetDiscordClient().signal_guild_member_list_update().connect(sigc::hide(sigc::mem_fun(*this, &StatusIndicator::CheckStatus)));
    auto cb = [this](Snowflake id, PresenceStatus status) {
        if (id == m_id) CheckStatus();
    };
    Abaddon::Get().GetDiscordClient().signal_presence_update().connect(sigc::track_obj(cb, *this));

    CheckStatus();
}

StatusIndicator::~StatusIndicator() {
}

void StatusIndicator::CheckStatus() {
    const auto status = Abaddon::Get().GetDiscordClient().GetUserStatus(m_id);
    if (status.has_value()) {
        switch (*status) {
            case PresenceStatus::Online:
                m_color = OnlineColor;
                break;
            case PresenceStatus::Offline:
                m_color = OfflineColor;
                break;
            case PresenceStatus::DND:
                m_color = DNDColor;
                break;
            case PresenceStatus::Idle:
                m_color = IdleColor;
                break;
        }
    } else {
        m_color = OfflineColor;
    }

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

    cr->set_source_rgb(m_color.get_red(), m_color.get_green(), m_color.get_blue());
    cr->arc(width / 2, height / 2, width / 3, 0.0, 2 * (4 * std::atan(1)));
    cr->close_path();
    cr->fill_preserve();
    cr->stroke();

    return true;
}
