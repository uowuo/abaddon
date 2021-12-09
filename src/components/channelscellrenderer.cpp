#include "channelscellrenderer.hpp"
#include "abaddon.hpp"
#include <gtkmm.h>
#include "unreadrenderer.hpp"

CellRendererChannels::CellRendererChannels()
    : Glib::ObjectBase(typeid(CellRendererChannels))
    , Gtk::CellRenderer()
    , m_property_type(*this, "render-type")
    , m_property_id(*this, "id")
    , m_property_name(*this, "name")
    , m_property_pixbuf(*this, "pixbuf")
    , m_property_pixbuf_animation(*this, "pixbuf-animation")
    , m_property_expanded(*this, "expanded")
    , m_property_nsfw(*this, "nsfw") {
    property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
    property_xpad() = 2;
    property_ypad() = 2;
    m_property_name.get_proxy().signal_changed().connect([this] {
        m_renderer_text.property_markup() = m_property_name;
    });
}

CellRendererChannels::~CellRendererChannels() {
}

Glib::PropertyProxy<RenderType> CellRendererChannels::property_type() {
    return m_property_type.get_proxy();
}

Glib::PropertyProxy<uint64_t> CellRendererChannels::property_id() {
    return m_property_id.get_proxy();
}

Glib::PropertyProxy<Glib::ustring> CellRendererChannels::property_name() {
    return m_property_name.get_proxy();
}

Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf>> CellRendererChannels::property_icon() {
    return m_property_pixbuf.get_proxy();
}

Glib::PropertyProxy<Glib::RefPtr<Gdk::PixbufAnimation>> CellRendererChannels::property_icon_animation() {
    return m_property_pixbuf_animation.get_proxy();
}

Glib::PropertyProxy<bool> CellRendererChannels::property_expanded() {
    return m_property_expanded.get_proxy();
}

Glib::PropertyProxy<bool> CellRendererChannels::property_nsfw() {
    return m_property_nsfw.get_proxy();
}

void CellRendererChannels::get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_width_vfunc_guild(widget, minimum_width, natural_width);
        case RenderType::Category:
            return get_preferred_width_vfunc_category(widget, minimum_width, natural_width);
        case RenderType::TextChannel:
            return get_preferred_width_vfunc_channel(widget, minimum_width, natural_width);
        case RenderType::Thread:
            return get_preferred_width_vfunc_thread(widget, minimum_width, natural_width);
        case RenderType::DMHeader:
            return get_preferred_width_vfunc_dmheader(widget, minimum_width, natural_width);
        case RenderType::DM:
            return get_preferred_width_vfunc_dm(widget, minimum_width, natural_width);
    }
}

void CellRendererChannels::get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_width_for_height_vfunc_guild(widget, height, minimum_width, natural_width);
        case RenderType::Category:
            return get_preferred_width_for_height_vfunc_category(widget, height, minimum_width, natural_width);
        case RenderType::TextChannel:
            return get_preferred_width_for_height_vfunc_channel(widget, height, minimum_width, natural_width);
        case RenderType::Thread:
            return get_preferred_width_for_height_vfunc_thread(widget, height, minimum_width, natural_width);
        case RenderType::DMHeader:
            return get_preferred_width_for_height_vfunc_dmheader(widget, height, minimum_width, natural_width);
        case RenderType::DM:
            return get_preferred_width_for_height_vfunc_dm(widget, height, minimum_width, natural_width);
    }
}

void CellRendererChannels::get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_height_vfunc_guild(widget, minimum_height, natural_height);
        case RenderType::Category:
            return get_preferred_height_vfunc_category(widget, minimum_height, natural_height);
        case RenderType::TextChannel:
            return get_preferred_height_vfunc_channel(widget, minimum_height, natural_height);
        case RenderType::Thread:
            return get_preferred_height_vfunc_thread(widget, minimum_height, natural_height);
        case RenderType::DMHeader:
            return get_preferred_height_vfunc_dmheader(widget, minimum_height, natural_height);
        case RenderType::DM:
            return get_preferred_height_vfunc_dm(widget, minimum_height, natural_height);
    }
}

void CellRendererChannels::get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_height_for_width_vfunc_guild(widget, width, minimum_height, natural_height);
        case RenderType::Category:
            return get_preferred_height_for_width_vfunc_category(widget, width, minimum_height, natural_height);
        case RenderType::TextChannel:
            return get_preferred_height_for_width_vfunc_channel(widget, width, minimum_height, natural_height);
        case RenderType::Thread:
            return get_preferred_height_for_width_vfunc_thread(widget, width, minimum_height, natural_height);
        case RenderType::DMHeader:
            return get_preferred_height_for_width_vfunc_dmheader(widget, width, minimum_height, natural_height);
        case RenderType::DM:
            return get_preferred_height_for_width_vfunc_dm(widget, width, minimum_height, natural_height);
    }
}

void CellRendererChannels::render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return render_vfunc_guild(cr, widget, background_area, cell_area, flags);
        case RenderType::Category:
            return render_vfunc_category(cr, widget, background_area, cell_area, flags);
        case RenderType::TextChannel:
            return render_vfunc_channel(cr, widget, background_area, cell_area, flags);
        case RenderType::Thread:
            return render_vfunc_thread(cr, widget, background_area, cell_area, flags);
        case RenderType::DMHeader:
            return render_vfunc_dmheader(cr, widget, background_area, cell_area, flags);
        case RenderType::DM:
            return render_vfunc_dm(cr, widget, background_area, cell_area, flags);
    }
}

// guild functions

void CellRendererChannels::get_preferred_width_vfunc_guild(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    int pixbuf_width = 0;

    if (auto pixbuf = m_property_pixbuf_animation.get_value())
        pixbuf_width = pixbuf->get_width();
    else if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_width = pixbuf->get_width();

    int text_min, text_nat;
    m_renderer_text.get_preferred_width(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_width = std::max(text_min, pixbuf_width) + xpad * 2;
    natural_width = std::max(text_nat, pixbuf_width) + xpad * 2;
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_guild(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc_guild(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_guild(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    int pixbuf_height = 0;
    if (auto pixbuf = m_property_pixbuf_animation.get_value())
        pixbuf_height = pixbuf->get_height();
    else if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_height = pixbuf->get_height();

    int text_min, text_nat;
    m_renderer_text.get_preferred_height(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_height = std::max(text_min, pixbuf_height) + ypad * 2;
    natural_height = std::max(text_nat, pixbuf_height) + ypad * 2;
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_guild(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc_guild(widget, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_guild(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    Gtk::Requisition minimum, natural;
    get_preferred_size(widget, minimum, natural);

    int pixbuf_w, pixbuf_h = 0;
    if (auto pixbuf = m_property_pixbuf_animation.get_value()) {
        pixbuf_w = pixbuf->get_width();
        pixbuf_h = pixbuf->get_height();
    } else if (auto pixbuf = m_property_pixbuf.get_value()) {
        pixbuf_w = pixbuf->get_width();
        pixbuf_h = pixbuf->get_height();
    }

    const double icon_w = pixbuf_w;
    const double icon_h = pixbuf_h;
    const double icon_x = background_area.get_x() + 3;
    const double icon_y = background_area.get_y() + background_area.get_height() / 2.0 - icon_h / 2.0;

    const double text_x = icon_x + icon_w + 5.0;
    const double text_y = background_area.get_y() + background_area.get_height() / 2.0 - text_natural.height / 2.0;
    const double text_w = text_natural.width;
    const double text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);

    const bool hover_only = Abaddon::Get().GetSettings().AnimatedGuildHoverOnly;
    const bool is_hovered = flags & Gtk::CELL_RENDERER_PRELIT;
    auto anim = m_property_pixbuf_animation.get_value();

    // kinda gross
    if (anim) {
        auto map_iter = m_pixbuf_anim_iters.find(anim);
        if (map_iter == m_pixbuf_anim_iters.end())
            m_pixbuf_anim_iters[anim] = anim->get_iter(nullptr);
        auto pb_iter = m_pixbuf_anim_iters.at(anim);

        const auto cb = [this, &widget, anim, icon_x, icon_y, icon_w, icon_h] {
            if (m_pixbuf_anim_iters.at(anim)->advance())
                widget.queue_draw_area(icon_x, icon_y, icon_w, icon_h);
        };

        if ((hover_only && is_hovered) || !hover_only)
            Glib::signal_timeout().connect_once(sigc::track_obj(cb, widget), pb_iter->get_delay_time());
        if (hover_only && !is_hovered)
            m_pixbuf_anim_iters[anim] = anim->get_iter(nullptr);

        Gdk::Cairo::set_source_pixbuf(cr, pb_iter->get_pixbuf(), icon_x, icon_y);
        cr->rectangle(icon_x, icon_y, icon_w, icon_h);
        cr->fill();
    } else if (auto pixbuf = m_property_pixbuf.get_value()) {
        Gdk::Cairo::set_source_pixbuf(cr, pixbuf, icon_x, icon_y);
        cr->rectangle(icon_x, icon_y, icon_w, icon_h);
        cr->fill();
    }

    UnreadRenderer::RenderUnreadOnGuild(m_property_id.get_value(), cr, background_area, cell_area);
}

// category

void CellRendererChannels::get_preferred_width_vfunc_category(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_category(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_category(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_category(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_category(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    // todo: figure out how Gtk::Arrow is rendered because i like it better :^)
    constexpr static int len = 5;
    int x1, y1, x2, y2, x3, y3;
    if (property_expanded()) {
        x1 = background_area.get_x() + 7;
        y1 = background_area.get_y() + background_area.get_height() / 2 - len;
        x2 = background_area.get_x() + 7 + len;
        y2 = background_area.get_y() + background_area.get_height() / 2 + len;
        x3 = background_area.get_x() + 7 + len * 2;
        y3 = background_area.get_y() + background_area.get_height() / 2 - len;
    } else {
        x1 = background_area.get_x() + 7;
        y1 = background_area.get_y() + background_area.get_height() / 2 - len;
        x2 = background_area.get_x() + 7 + len * 2;
        y2 = background_area.get_y() + background_area.get_height() / 2;
        x3 = background_area.get_x() + 7;
        y3 = background_area.get_y() + background_area.get_height() / 2 + len;
    }
    cr->move_to(x1, y1);
    cr->line_to(x2, y2);
    cr->line_to(x3, y3);
    const auto expander_color = Gdk::RGBA(Abaddon::Get().GetSettings().ChannelsExpanderColor);
    cr->set_source_rgb(expander_color.get_red(), expander_color.get_green(), expander_color.get_blue());
    cr->stroke();

    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    const int text_x = background_area.get_x() + 22;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - text_natural.height / 2;
    const int text_w = text_natural.width;
    const int text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}

// text channel

void CellRendererChannels::get_preferred_width_vfunc_channel(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_channel(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_channel(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_channel(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_channel(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition minimum_size, natural_size;
    m_renderer_text.get_preferred_size(widget, minimum_size, natural_size);

    const int text_x = background_area.get_x() + 21;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - natural_size.height / 2;
    const int text_w = natural_size.width;
    const int text_h = natural_size.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    const auto nsfw_color = Gdk::RGBA(Abaddon::Get().GetSettings().NSFWChannelColor);
    if (m_property_nsfw.get_value())
        m_renderer_text.property_foreground_rgba() = nsfw_color;
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
    // setting property_foreground_rgba() sets this to true which makes non-nsfw cells use the property too which is bad
    // so unset it
    m_renderer_text.property_foreground_set() = false;

    UnreadRenderer::RenderUnreadOnChannel(m_property_id.get_value(), cr, background_area, cell_area);
}

// thread

void CellRendererChannels::get_preferred_width_vfunc_thread(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_thread(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc_thread(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_thread(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_thread(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc_thread(widget, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_thread(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition minimum_size, natural_size;
    m_renderer_text.get_preferred_size(widget, minimum_size, natural_size);

    const int text_x = background_area.get_x() + 26;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - natural_size.height / 2;
    const int text_w = natural_size.width;
    const int text_h = natural_size.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}

// dm header

void CellRendererChannels::get_preferred_width_vfunc_dmheader(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_dmheader(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_dmheader(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_dmheader(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_dmheader(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    // gdk::rectangle more like gdk::stupid
    Gdk::Rectangle text_cell_area(
        cell_area.get_x() + 9, cell_area.get_y(), // maybe theres a better way to align this ?
        cell_area.get_width(), cell_area.get_height());
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}

// dm (basically the same thing as guild)

void CellRendererChannels::get_preferred_width_vfunc_dm(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    int pixbuf_width = 0;
    if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_width = pixbuf->get_width();

    int text_min, text_nat;
    m_renderer_text.get_preferred_width(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_width = std::max(text_min, pixbuf_width) + xpad * 2;
    natural_width = std::max(text_nat, pixbuf_width) + xpad * 2;
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_dm(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc_guild(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_dm(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    int pixbuf_height = 0;
    if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_height = pixbuf->get_height();

    int text_min, text_nat;
    m_renderer_text.get_preferred_height(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_height = std::max(text_min, pixbuf_height) + ypad * 2;
    natural_height = std::max(text_nat, pixbuf_height) + ypad * 2;
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_dm(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc_guild(widget, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_dm(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    Gtk::Requisition minimum, natural;
    get_preferred_size(widget, minimum, natural);

    auto pixbuf = m_property_pixbuf.get_value();

    const double icon_w = pixbuf->get_width();
    const double icon_h = pixbuf->get_height();
    const double icon_x = background_area.get_x() + 2;
    const double icon_y = background_area.get_y() + background_area.get_height() / 2.0 - icon_h / 2.0;

    const double text_x = icon_x + icon_w + 5.0;
    const double text_y = background_area.get_y() + background_area.get_height() / 2.0 - text_natural.height / 2.0;
    const double text_w = text_natural.width;
    const double text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);

    Gdk::Cairo::set_source_pixbuf(cr, m_property_pixbuf.get_value(), icon_x, icon_y);
    cr->rectangle(icon_x, icon_y, icon_w, icon_h);
    cr->fill();
}
