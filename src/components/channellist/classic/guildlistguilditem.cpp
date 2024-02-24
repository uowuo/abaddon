#include "guildlistguilditem.hpp"

#include "abaddon.hpp"

GuildListGuildItem::GuildListGuildItem(const GuildData &guild)
    : ID(guild.ID) {
    get_style_context()->add_class("classic-guild-list-guild");

    m_image.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(48);

    add(m_box);
    m_box.pack_start(m_image);
    show_all_children();

    set_tooltip_text(guild.Name);

    UpdateIcon();

    Abaddon::Get().GetDiscordClient().signal_message_create().connect(sigc::mem_fun(*this, &GuildListGuildItem::OnMessageCreate));
    Abaddon::Get().GetDiscordClient().signal_message_ack().connect(sigc::mem_fun(*this, &GuildListGuildItem::OnMessageAck));

    CheckUnreadStatus();
}

void GuildListGuildItem::UpdateIcon() {
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(ID);
    if (!guild.has_value() || !guild->HasIcon()) return;
    Abaddon::Get().GetImageManager().LoadFromURL(guild->GetIconURL("png", "64"), sigc::mem_fun(*this, &GuildListGuildItem::OnIconFetched));
}

void GuildListGuildItem::OnIconFetched(const Glib::RefPtr<Gdk::Pixbuf> &pb) {
    m_image.property_pixbuf() = pb->scale_simple(48, 48, Gdk::INTERP_BILINEAR);
}

void GuildListGuildItem::OnMessageCreate(const Message &msg) {
    if (msg.GuildID.has_value() && *msg.GuildID == ID) CheckUnreadStatus();
}

void GuildListGuildItem::OnMessageAck(const MessageAckData &data) {
    CheckUnreadStatus();
}

void GuildListGuildItem::CheckUnreadStatus() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (!Abaddon::Get().GetSettings().Unreads) return;
    int mentions;
    if (!discord.IsGuildMuted(ID) && discord.GetUnreadStateForGuild(ID, mentions)) {
        get_style_context()->add_class("has-unread");
    } else {
        get_style_context()->remove_class("has-unread");
    }
}
