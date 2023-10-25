#include "guildlistguilditem.hpp"

GuildListGuildItem::GuildListGuildItem(const GuildData &guild)
    : ID(guild.ID) {
    m_image.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(48);
    add(m_image);
    show_all_children();

    signal_button_press_event().connect([this](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            printf("Click %llu\n", (uint64_t)ID);
        }
        return true;
    });

    set_tooltip_text(guild.Name);

    UpdateIcon();
}

void GuildListGuildItem::UpdateIcon() {
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(ID);
    if (!guild.has_value()) return;
    Abaddon::Get().GetImageManager().LoadFromURL(guild->GetIconURL("png", "64"), sigc::mem_fun(*this, &GuildListGuildItem::OnIconFetched));
}

void GuildListGuildItem::OnIconFetched(const Glib::RefPtr<Gdk::Pixbuf> &pb) {
    m_image.property_pixbuf() = pb->scale_simple(48, 48, Gdk::INTERP_BILINEAR);
}