#include "cellrendererchannels.hpp"
#include <gdkmm/general.h>
#include "abaddon.hpp"

constexpr static int MentionsRightPad = 7;
#ifndef M_PI
constexpr static double M_PI = 3.14159265358979;
#endif
constexpr static double M_PI_H = M_PI / 2.0;
constexpr static double M_PI_3_2 = M_PI * 3.0 / 2.0;

void AddUnreadIndicator(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area) {
    static const auto color_setting = Gdk::RGBA(Abaddon::Get().GetSettings().UnreadIndicatorColor);

    const auto color = color_setting.get_alpha_u() > 0 ? color_setting : widget.get_style_context()->get_background_color(Gtk::STATE_FLAG_SELECTED);

    cr->set_source_rgb(color.get_red(), color.get_green(), color.get_blue());
    const auto x = background_area.get_x();
    const auto y = background_area.get_y();
    const auto w = background_area.get_width();
    const auto h = background_area.get_height();
    cr->rectangle(x, y, 3, h);
    cr->fill();
}

void RenderExpander(int x_offset, const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, bool is_expanded) {
    constexpr static int len = 5;
    int x1, y1, x2, y2, x3, y3;
    if (is_expanded) {
        x1 = background_area.get_x() + x_offset;
        y1 = background_area.get_y() + background_area.get_height() / 2 - len;
        x2 = background_area.get_x() + x_offset + len;
        y2 = background_area.get_y() + background_area.get_height() / 2 + len;
        x3 = background_area.get_x() + x_offset + len * 2;
        y3 = background_area.get_y() + background_area.get_height() / 2 - len;
    } else {
        x1 = background_area.get_x() + x_offset;
        y1 = background_area.get_y() + background_area.get_height() / 2 - len;
        x2 = background_area.get_x() + x_offset + len * 2;
        y2 = background_area.get_y() + background_area.get_height() / 2;
        x3 = background_area.get_x() + x_offset;
        y3 = background_area.get_y() + background_area.get_height() / 2 + len;
    }
    cr->move_to(x1, y1);
    cr->line_to(x2, y2);
    cr->line_to(x3, y3);
    auto expander_color = Gdk::RGBA(Abaddon::Get().GetSettings().ChannelsExpanderColor);
    if (expander_color.get_alpha_u() == 0) {
        expander_color = widget.get_style_context()->get_background_color(Gtk::STATE_FLAG_SELECTED);
    }
    cr->set_source_rgb(expander_color.get_red(), expander_color.get_green(), expander_color.get_blue());
    cr->stroke();
}

CellRendererChannels::CellRendererChannels()
    : Glib::ObjectBase(typeid(CellRendererChannels))
    , Gtk::CellRenderer()
    , m_property_type(*this, "render-type")
    , m_property_id(*this, "id")
    , m_property_name(*this, "name")
    , m_property_pixbuf(*this, "pixbuf")
    , m_property_pixbuf_animation(*this, "pixbuf-animation")
    , m_property_expanded(*this, "expanded")
    , m_property_nsfw(*this, "nsfw")
    , m_property_color(*this, "color")
    , m_property_voice_state(*this, "voice-state") {
    property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
    property_xpad() = 2;
    property_ypad() = 2;
    m_property_name.get_proxy().signal_changed().connect([this] {
        m_renderer_text.property_markup() = m_property_name;
    });
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

Glib::PropertyProxy<std::optional<Gdk::RGBA>> CellRendererChannels::property_color() {
    return m_property_color.get_proxy();
}

Glib::PropertyProxy<VoiceStateFlags> CellRendererChannels::property_voice_state() {
    return m_property_voice_state.get_proxy();
}

void CellRendererChannels::get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case RenderType::Folder:
            return get_preferred_width_vfunc_folder(widget, minimum_width, natural_width);
        case RenderType::Guild:
            return get_preferred_width_vfunc_guild(widget, minimum_width, natural_width);
        case RenderType::Category:
            return get_preferred_width_vfunc_category(widget, minimum_width, natural_width);
        case RenderType::TextChannel:
            return get_preferred_width_vfunc_channel(widget, minimum_width, natural_width);
        case RenderType::Thread:
            return get_preferred_width_vfunc_thread(widget, minimum_width, natural_width);
#ifdef WITH_VOICE
        case RenderType::VoiceChannel:
            return get_preferred_width_vfunc_voice_channel(widget, minimum_width, natural_width);
        case RenderType::VoiceParticipant:
            return get_preferred_width_vfunc_voice_participant(widget, minimum_width, natural_width);
#endif
        case RenderType::DMHeader:
            return get_preferred_width_vfunc_dmheader(widget, minimum_width, natural_width);
        case RenderType::DM:
            return get_preferred_width_vfunc_dm(widget, minimum_width, natural_width);
    }
}

void CellRendererChannels::get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case RenderType::Folder:
            return get_preferred_width_for_height_vfunc_folder(widget, height, minimum_width, natural_width);
        case RenderType::Guild:
            return get_preferred_width_for_height_vfunc_guild(widget, height, minimum_width, natural_width);
        case RenderType::Category:
            return get_preferred_width_for_height_vfunc_category(widget, height, minimum_width, natural_width);
        case RenderType::TextChannel:
            return get_preferred_width_for_height_vfunc_channel(widget, height, minimum_width, natural_width);
        case RenderType::Thread:
            return get_preferred_width_for_height_vfunc_thread(widget, height, minimum_width, natural_width);
#ifdef WITH_VOICE
        case RenderType::VoiceChannel:
            return get_preferred_width_for_height_vfunc_voice_channel(widget, height, minimum_width, natural_width);
        case RenderType::VoiceParticipant:
            return get_preferred_width_for_height_vfunc_voice_participant(widget, height, minimum_width, natural_width);
#endif
        case RenderType::DMHeader:
            return get_preferred_width_for_height_vfunc_dmheader(widget, height, minimum_width, natural_width);
        case RenderType::DM:
            return get_preferred_width_for_height_vfunc_dm(widget, height, minimum_width, natural_width);
    }
}

void CellRendererChannels::get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case RenderType::Folder:
            return get_preferred_height_vfunc_folder(widget, minimum_height, natural_height);
        case RenderType::Guild:
            return get_preferred_height_vfunc_guild(widget, minimum_height, natural_height);
        case RenderType::Category:
            return get_preferred_height_vfunc_category(widget, minimum_height, natural_height);
        case RenderType::TextChannel:
            return get_preferred_height_vfunc_channel(widget, minimum_height, natural_height);
        case RenderType::Thread:
            return get_preferred_height_vfunc_thread(widget, minimum_height, natural_height);
#ifdef WITH_VOICE
        case RenderType::VoiceChannel:
            return get_preferred_height_vfunc_voice_channel(widget, minimum_height, natural_height);
        case RenderType::VoiceParticipant:
            return get_preferred_height_vfunc_voice_participant(widget, minimum_height, natural_height);
#endif
        case RenderType::DMHeader:
            return get_preferred_height_vfunc_dmheader(widget, minimum_height, natural_height);
        case RenderType::DM:
            return get_preferred_height_vfunc_dm(widget, minimum_height, natural_height);
    }
}

void CellRendererChannels::get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case RenderType::Folder:
            return get_preferred_height_for_width_vfunc_folder(widget, width, minimum_height, natural_height);
        case RenderType::Guild:
            return get_preferred_height_for_width_vfunc_guild(widget, width, minimum_height, natural_height);
        case RenderType::Category:
            return get_preferred_height_for_width_vfunc_category(widget, width, minimum_height, natural_height);
        case RenderType::TextChannel:
            return get_preferred_height_for_width_vfunc_channel(widget, width, minimum_height, natural_height);
        case RenderType::Thread:
            return get_preferred_height_for_width_vfunc_thread(widget, width, minimum_height, natural_height);
#ifdef WITH_VOICE
        case RenderType::VoiceChannel:
            return get_preferred_height_for_width_vfunc_voice_channel(widget, width, minimum_height, natural_height);
        case RenderType::VoiceParticipant:
            return get_preferred_height_for_width_vfunc_voice_participant(widget, width, minimum_height, natural_height);
#endif
        case RenderType::DMHeader:
            return get_preferred_height_for_width_vfunc_dmheader(widget, width, minimum_height, natural_height);
        case RenderType::DM:
            return get_preferred_height_for_width_vfunc_dm(widget, width, minimum_height, natural_height);
    }
}

void CellRendererChannels::render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    switch (m_property_type.get_value()) {
        case RenderType::Folder:
            return render_vfunc_folder(cr, widget, background_area, cell_area, flags);
        case RenderType::Guild:
            return render_vfunc_guild(cr, widget, background_area, cell_area, flags);
        case RenderType::Category:
            return render_vfunc_category(cr, widget, background_area, cell_area, flags);
        case RenderType::TextChannel:
            return render_vfunc_channel(cr, widget, background_area, cell_area, flags);
        case RenderType::Thread:
            return render_vfunc_thread(cr, widget, background_area, cell_area, flags);
#ifdef WITH_VOICE
        case RenderType::VoiceChannel:
            return render_vfunc_voice_channel(cr, widget, background_area, cell_area, flags);
        case RenderType::VoiceParticipant:
            return render_vfunc_voice_participant(cr, widget, background_area, cell_area, flags);
#endif
        case RenderType::DMHeader:
            return render_vfunc_dmheader(cr, widget, background_area, cell_area, flags);
        case RenderType::DM:
            return render_vfunc_dm(cr, widget, background_area, cell_area, flags);
    }
}

// folder functions

void CellRendererChannels::get_preferred_width_vfunc_folder(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_folder(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_folder(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_folder(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_folder(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    RenderExpander(7, cr, widget, background_area, property_expanded());
    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    const int text_x = background_area.get_x() + 22;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - text_natural.height / 2;
    const int text_w = text_natural.width;
    const int text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    if (m_property_color.get_value().has_value()) {
        m_renderer_text.property_foreground_rgba() = *m_property_color.get_value();
    }
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
    m_renderer_text.property_foreground_set() = false;
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

    Gdk::Rectangle text_cell_area(static_cast<int>(text_x),
                                  static_cast<int>(text_y),
                                  static_cast<int>(text_w),
                                  static_cast<int>(text_h));

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
                widget.queue_draw_area(
                    static_cast<int>(icon_x),
                    static_cast<int>(icon_y),
                    static_cast<int>(icon_w),
                    static_cast<int>(icon_h));
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

    // unread
    if (!Abaddon::Get().GetSettings().Unreads) return;

    const auto id = m_property_id.get_value();

    auto &discord = Abaddon::Get().GetDiscordClient();
    int total_mentions;
    const auto has_unread = discord.GetUnreadStateForGuild(id, total_mentions);

    if (has_unread && !discord.IsGuildMuted(id)) {
        auto area = background_area;
        area.set_y(area.get_y() + area.get_height() / 2.0 - 24.0 / 2.0);
        AddUnreadIndicator(cr, widget, area);
    }

    if (total_mentions < 1) return;
    auto *paned = dynamic_cast<Gtk::Paned *>(widget.get_ancestor(Gtk::Paned::get_type()));
    if (paned != nullptr) {
        const auto edge = std::min(paned->get_position(), background_area.get_width());

        unread_render_mentions(cr, widget, total_mentions, edge, background_area);
    }
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
    RenderExpander(7, cr, widget, background_area, property_expanded());

    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    const int text_x = background_area.get_x() + 22;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - text_natural.height / 2;
    const int text_w = text_natural.width;
    const int text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto id = m_property_id.get_value();
    if (!discord.IsChannelMuted(m_property_id.get_value())) {
        if (discord.GetUnreadChannelsCountForCategory(id) > 0) {
            AddUnreadIndicator(cr, widget, background_area);
        }
    }
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

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto id = m_property_id.get_value();
    const bool is_muted = discord.IsChannelMuted(id);

    static const auto nsfw_color = Gdk::RGBA(Abaddon::Get().GetSettings().NSFWChannelColor);

    auto color = widget.get_style_context()->get_color(Gtk::STATE_FLAG_NORMAL);
    if (property_nsfw()) color = nsfw_color;
    if (is_muted) color.set_alpha(0.6);

    m_renderer_text.property_foreground_rgba() = color;
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
    m_renderer_text.property_foreground_set() = false;

    // unread
    if (!Abaddon::Get().GetSettings().Unreads) return;

    const auto unread_state = discord.GetUnreadStateForChannel(id);
    if (unread_state < 0) return;

    if (!is_muted) {
        AddUnreadIndicator(cr, widget, background_area);
    }

    if (unread_state < 1) return;
    auto *paned = dynamic_cast<Gtk::Paned *>(widget.get_ancestor(Gtk::Paned::get_type()));
    if (paned != nullptr) {
        const auto edge = std::min(paned->get_position(), cell_area.get_width());

        unread_render_mentions(cr, widget, unread_state, edge, cell_area);
    }
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

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto id = m_property_id.get_value();
    const bool is_muted = discord.IsChannelMuted(id);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);

    // unread
    if (!Abaddon::Get().GetSettings().Unreads) return;

    const auto unread_state = discord.GetUnreadStateForChannel(id);
    if (unread_state < 0) return;

    if (!is_muted) {
        AddUnreadIndicator(cr, widget, background_area);
    }

    if (unread_state < 1) return;
    auto *paned = dynamic_cast<Gtk::Paned *>(widget.get_ancestor(Gtk::Paned::get_type()));
    if (paned != nullptr) {
        const auto edge = std::min(paned->get_position(), cell_area.get_width());

        unread_render_mentions(cr, widget, unread_state, edge, cell_area);
    }
}

#ifdef WITH_VOICE

// voice channel

void CellRendererChannels::get_preferred_width_vfunc_voice_channel(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_voice_channel(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_voice_channel(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_voice_channel(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_voice_channel(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    // channel name text
    Gtk::Requisition minimum_size, natural_size;
    m_renderer_text.get_preferred_size(widget, minimum_size, natural_size);

    const int text_x = background_area.get_x() + 35;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - natural_size.height / 2;
    const int text_w = natural_size.width;
    const int text_h = natural_size.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);

    // speaker character
    Pango::FontDescription font;
    font.set_family("sans 14");

    auto layout = widget.create_pango_layout("\U0001F50A");
    layout->set_font_description(font);
    layout->set_alignment(Pango::ALIGN_LEFT);
    cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
    int width, height;
    layout->get_pixel_size(width, height);
    cr->move_to(
        background_area.get_x() + 1,
        cell_area.get_y() + cell_area.get_height() / 2.0 - height / 2.0);
    layout->show_in_cairo_context(cr);

    RenderExpander(24, cr, widget, background_area, property_expanded());
}

// voice participant

void CellRendererChannels::get_preferred_width_vfunc_voice_participant(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_voice_participant(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_voice_participant(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_voice_participant(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_voice_participant(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    Gtk::Requisition minimum, natural;
    get_preferred_size(widget, minimum, natural);

    int pixbuf_w = 0;
    int pixbuf_h = 0;

    if (auto pixbuf = m_property_pixbuf.get_value()) {
        pixbuf_w = pixbuf->get_width();
        pixbuf_h = pixbuf->get_height();
    }

    const double icon_w = pixbuf_w;
    const double icon_h = pixbuf_h;
    const double icon_x = background_area.get_x() + 28;
    const double icon_y = background_area.get_y() + background_area.get_height() / 2.0 - icon_h / 2.0;

    const double text_x = icon_x + icon_w + 5.0;
    const double text_y = background_area.get_y() + background_area.get_height() / 2.0 - text_natural.height / 2.0;
    const double text_w = text_natural.width;
    const double text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);
    m_renderer_text.property_scale() = Pango::SCALE_SMALL;
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
    m_renderer_text.property_scale_set() = false;

    if (auto pixbuf = m_property_pixbuf.get_value()) {
        Gdk::Cairo::set_source_pixbuf(cr, pixbuf, icon_x, icon_y);
        cr->rectangle(icon_x, icon_y, icon_w, icon_h);
        cr->fill();
    }

    auto *paned = dynamic_cast<Gtk::Paned *>(widget.get_ancestor(Gtk::Paned::get_type()));
    if (paned != nullptr) {
        const auto edge = std::min(paned->get_position(), background_area.get_width());

        const static std::array<std::pair<VoiceStateFlags, Glib::ustring>, 3> icon_order = { {
            { VoiceStateFlags::SelfMute | VoiceStateFlags::Mute, "microphone-disabled-symbolic" },
            { VoiceStateFlags::SelfDeaf | VoiceStateFlags::Deaf, "audio-volume-muted-symbolic" },
            { VoiceStateFlags::SelfVideo, "camera-web-symbolic" },
        } };

        constexpr static int IconSize = 18;
        constexpr static int IconPad = 2;

        const VoiceStateFlags voice_flags = m_property_voice_state.get_value();

        int offset = 0;
        for (auto iter = icon_order.rbegin(); iter != icon_order.rend(); iter++) {
            const auto &[flag, icon] = *iter;
            if ((voice_flags & flag) == VoiceStateFlags::Clear) continue;

            const double icon_w = 18;
            const double icon_h = 18;
            const double icon_x = background_area.get_x() + edge - icon_w + offset;
            const double icon_y = background_area.get_y() + background_area.get_height() / 2 - icon_h / 2;
            Gdk::Rectangle icon_cell_area(icon_x, icon_y, icon_w, icon_h);

            offset -= (IconSize + IconPad);

            const bool is_server_mute = (voice_flags & VoiceStateFlags::Mute) == VoiceStateFlags::Mute;
            const bool is_server_deaf = (voice_flags & VoiceStateFlags::Deaf) == VoiceStateFlags::Deaf;
            auto context = widget.get_style_context();
            if (is_server_mute || is_server_deaf) {
                context->context_save();
                context->add_class("voice-state-server");
            }

            m_renderer_pixbuf.property_icon_name() = icon;
            m_renderer_pixbuf.render(cr, widget, background_area, icon_cell_area, flags);

            if (is_server_mute || is_server_deaf) {
                context->context_restore();
            }
        }
    }
}

#endif

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

    if (!Abaddon::Get().GetSettings().Unreads) return;

    auto *paned = dynamic_cast<Gtk::Paned *>(widget.get_ancestor(Gtk::Paned::get_type()));
    if (paned != nullptr) {
        const auto edge = std::min(paned->get_position(), background_area.get_width());
        if (const auto unread = Abaddon::Get().GetDiscordClient().GetUnreadDMsCount(); unread > 0)
            unread_render_mentions(cr, widget, unread, edge, background_area);
    }
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
    const double icon_x = background_area.get_x() + 3;
    const double icon_y = background_area.get_y() + background_area.get_height() / 2.0 - icon_h / 2.0;

    const double text_x = icon_x + icon_w + 6.0;
    const double text_y = background_area.get_y() + background_area.get_height() / 2.0 - text_natural.height / 2.0;
    const double text_w = text_natural.width;
    const double text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(static_cast<int>(text_x),
                                  static_cast<int>(text_y),
                                  static_cast<int>(text_w),
                                  static_cast<int>(text_h));

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto id = m_property_id.get_value();
    const bool is_muted = discord.IsChannelMuted(id);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);

    Gdk::Cairo::set_source_pixbuf(cr, m_property_pixbuf.get_value(), icon_x, icon_y);
    cr->rectangle(icon_x, icon_y, icon_w, icon_h);
    cr->fill();

    // unread
    if (!Abaddon::Get().GetSettings().Unreads) return;

    const auto unread_state = discord.GetUnreadStateForChannel(id);
    if (unread_state < 0) return;

    if (!is_muted) {
        AddUnreadIndicator(cr, widget, background_area);
    }
}

void CellRendererChannels::cairo_path_rounded_rect(const Cairo::RefPtr<Cairo::Context> &cr, double x, double y, double w, double h, double r) {
    const double degrees = M_PI / 180.0;

    cr->begin_new_sub_path();
    cr->arc(x + w - r, y + r, r, -M_PI_H, 0);
    cr->arc(x + w - r, y + h - r, r, 0, M_PI_H);
    cr->arc(x + r, y + h - r, r, M_PI_H, M_PI);
    cr->arc(x + r, y + r, r, M_PI, M_PI_3_2);
    cr->close_path();
}

void CellRendererChannels::unread_render_mentions(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, int mentions, int edge, const Gdk::Rectangle &cell_area) {
    Pango::FontDescription font;
    font.set_family("sans 14");
    // font.set_weight(Pango::WEIGHT_BOLD);

    auto layout = widget.create_pango_layout(std::to_string(mentions));
    layout->set_font_description(font);
    layout->set_alignment(Pango::ALIGN_RIGHT);

    int width, height;
    layout->get_pixel_size(width, height);
    {
        static const auto badge_setting = Gdk::RGBA(Abaddon::Get().GetSettings().MentionBadgeColor);
        static const auto text_setting = Gdk::RGBA(Abaddon::Get().GetSettings().MentionBadgeTextColor);

        auto bg = badge_setting.get_alpha_u() > 0 ? badge_setting : widget.get_style_context()->get_background_color(Gtk::STATE_FLAG_SELECTED);
        auto text = text_setting.get_alpha_u() > 0 ? text_setting : widget.get_style_context()->get_color(Gtk::STATE_FLAG_SELECTED);

        const auto x = cell_area.get_x() + edge - width - MentionsRightPad;
        const auto y = cell_area.get_y() + cell_area.get_height() / 2.0 - height / 2.0 - 1;
        cairo_path_rounded_rect(cr, x - 4, y + 2, width + 8, height, 5);
        cr->set_source_rgb(bg.get_red(), bg.get_green(), bg.get_blue());
        cr->fill();
        cr->set_source_rgb(text.get_red(), text.get_green(), text.get_blue());
        cr->move_to(x, y);
        layout->show_in_cairo_context(cr);
    }
}
