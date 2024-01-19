#include "mutualguildspane.hpp"

#include "abaddon.hpp"

MutualGuildItem::MutualGuildItem(const MutualGuildData &guild)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
    , m_box(Gtk::ORIENTATION_VERTICAL) {
    get_style_context()->add_class("mutual-guild-item");
    m_name.get_style_context()->add_class("mutual-guild-item-name");
    m_icon.get_style_context()->add_class("mutual-guild-item-icon");

    m_icon.set_margin_end(10);

    // discord will return info (id + nick) for "deleted" guilds from this endpoint. strange !
    const auto data = Abaddon::Get().GetDiscordClient().GetGuild(guild.ID);
    if (data.has_value()) {
        auto &img = Abaddon::Get().GetImageManager();
        m_icon.property_pixbuf() = img.GetPlaceholder(24);
        if (data->HasIcon()) {
            if (data->HasAnimatedIcon() && Abaddon::Get().GetSettings().ShowAnimations) {
                auto cb = [this](const Glib::RefPtr<Gdk::PixbufAnimation> &pb) {
                    m_icon.property_pixbuf_animation() = pb;
                };
                img.LoadAnimationFromURL(data->GetIconURL("gif", "32"), 24, 24, sigc::track_obj(cb, *this));
            } else {
                auto cb = [this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
                    m_icon.property_pixbuf() = pb->scale_simple(24, 24, Gdk::INTERP_BILINEAR);
                };
                img.LoadFromURL(data->GetIconURL("png", "32"), sigc::track_obj(cb, *this));
            }
        }

        m_name.set_markup("<b>" + Glib::Markup::escape_text(data->Name) + "</b>");
    } else {
        m_icon.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(24);
        m_name.set_markup("<b>Unknown server</b>");
    }

    if (guild.Nick.has_value()) {
        m_nick = Gtk::manage(new Gtk::Label(*guild.Nick));
        m_nick->get_style_context()->add_class("mutual-guild-item-nick");
        m_nick->set_margin_start(5);
        m_nick->set_halign(Gtk::ALIGN_START);
        m_nick->set_single_line_mode(true);
        m_nick->set_ellipsize(Pango::ELLIPSIZE_END);
    }

    m_box.set_valign(Gtk::ALIGN_CENTER);

    m_box.add(m_name);
    if (m_nick != nullptr)
        m_box.add(*m_nick);
    add(m_icon);
    add(m_box);
    show_all_children();
}

UserMutualGuildsPane::UserMutualGuildsPane(Snowflake id)
    : UserID(id) {
    add(m_list);
    show_all_children();
}

void UserMutualGuildsPane::SetMutualGuilds(const std::vector<MutualGuildData> &guilds) {
    for (auto child : m_list.get_children())
        delete child;

    for (const auto &guild : guilds) {
        auto *item = Gtk::manage(new MutualGuildItem(guild));
        item->show();
        m_list.add(*item);
    }
}
