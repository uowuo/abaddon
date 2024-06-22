#include "mentionoverlay.hpp"

#include "misc/cairo.hpp"

#include "abaddon.hpp"

MentionOverlay::MentionOverlay(Snowflake guild_id)
    : m_guild_ids({ guild_id }) {
    Init();
}

MentionOverlay::MentionOverlay(const UserSettingsGuildFoldersEntry &folder)
    : m_guild_ids({ folder.GuildIDs.begin(), folder.GuildIDs.end() }) {
    Init();
}

void MentionOverlay::Init() {
    m_font.set_family("sans 14");
    m_layout = create_pango_layout("12");
    m_layout->set_font_description(m_font);
    m_layout->set_alignment(Pango::ALIGN_RIGHT);

    get_style_context()->add_class("classic-mention-overlay"); // fuck you

    set_hexpand(false);
    set_vexpand(false);

    signal_draw().connect(sigc::mem_fun(*this, &MentionOverlay::OnDraw));

    Abaddon::Get().GetDiscordClient().signal_message_ack().connect([this](const MessageAckData &data) {
        // fetching and checking guild id is probably more expensive than just forcing a redraw anyways
        queue_draw();
    });

    Abaddon::Get().GetDiscordClient().signal_message_create().connect([this](const Message &msg) {
        if (msg.GuildID.has_value() && m_guild_ids.find(*msg.GuildID) == m_guild_ids.end()) return;
        if (!msg.DoesMentionEveryone && msg.Mentions.empty() && msg.MentionRoles.empty()) return;
        queue_draw();
    });
}

bool MentionOverlay::OnDraw(const Cairo::RefPtr<Cairo::Context> &cr) {
    int total_mentions = 0;
    for (auto guild_id : m_guild_ids) {
        int mentions;
        Abaddon::Get().GetDiscordClient().GetUnreadStateForGuild(guild_id, mentions);
        total_mentions += mentions;
    }
    if (total_mentions == 0) return true;
    m_layout->set_text(std::to_string(total_mentions));

    const int width = get_allocated_width();
    const int height = std::min(get_allocated_height(), 48); // cope

    int lw, lh;
    m_layout->get_pixel_size(lw, lh);
    {
        static const auto badge_setting = Gdk::RGBA(Abaddon::Get().GetSettings().MentionBadgeColor);
        static const auto text_setting = Gdk::RGBA(Abaddon::Get().GetSettings().MentionBadgeTextColor);

        auto bg = badge_setting.get_alpha_u() > 0 ? badge_setting : get_style_context()->get_background_color(Gtk::STATE_FLAG_SELECTED);
        auto text = text_setting.get_alpha_u() > 0 ? text_setting : get_style_context()->get_color(Gtk::STATE_FLAG_SELECTED);

        const auto x = width - lw - 5;
        const auto y = height - lh - 1;
        CairoUtil::PathRoundedRect(cr, x - 4, y + 2, lw + 8, lh, 5);
        cr->set_source_rgb(bg.get_red(), bg.get_green(), bg.get_blue());
        cr->fill();
        cr->set_source_rgb(text.get_red(), text.get_green(), text.get_blue());
        cr->move_to(x, y);
        m_layout->show_in_cairo_context(cr);
    }

    return true;
}
