#include "ratelimitindicator.hpp"
#include "../abaddon.hpp"
#include <filesystem>

RateLimitIndicator::RateLimitIndicator()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL) {
    m_label.set_text("");
    m_label.set_ellipsize(Pango::ELLIPSIZE_START);
    m_label.set_valign(Gtk::ALIGN_END);
    get_style_context()->add_class("ratelimit-indicator");

    m_img.set_margin_start(7);

    add(m_label);
    add(m_img);
    m_label.show();

    if (std::filesystem::exists("./res/clock.png")) {
        try {
            const auto pixbuf = Gdk::Pixbuf::create_from_file("./res/clock.png");
            int w, h;
            GetImageDimensions(pixbuf->get_width(), pixbuf->get_height(), w, h, 20, 10);
            m_img.property_pixbuf() = pixbuf->scale_simple(w, h, Gdk::INTERP_BILINEAR);
        } catch (...) {}
    }

    Abaddon::Get().GetDiscordClient().signal_message_create().connect(sigc::mem_fun(*this, &RateLimitIndicator::OnMessageCreate));
}

void RateLimitIndicator::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
    const auto channel = *Abaddon::Get().GetDiscordClient().GetChannel(m_active_channel);
    if (channel.RateLimitPerUser.has_value())
        m_rate_limit = *channel.RateLimitPerUser;
    else
        m_rate_limit = 0;

    UpdateIndicator();
}

bool RateLimitIndicator::CanSpeak() const {
    const auto rate_limit = GetRateLimit();
    if (rate_limit == 0) return true;

    const auto it = m_times.find(m_active_channel);
    if (it == m_times.end())
        return true;

    const auto now = std::chrono::steady_clock::now();
    const auto sec_diff = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
    return sec_diff >= GetRateLimit();
}

int RateLimitIndicator::GetTimeLeft() const {
    if (CanSpeak()) return 0;

    auto it = m_times.find(m_active_channel);
    if (it == m_times.end()) return 0;

    const auto now = std::chrono::steady_clock::now();
    const auto sec_diff = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();

    if (sec_diff >= GetRateLimit())
        return 0;
    else
        return GetRateLimit() - sec_diff;
}

int RateLimitIndicator::GetRateLimit() const {
    return m_rate_limit;
}

bool RateLimitIndicator::UpdateIndicator() {
    if (const auto rate_limit = GetRateLimit(); rate_limit != 0) {
        m_img.show();

        auto &discord = Abaddon::Get().GetDiscordClient();
        if (discord.HasAnyChannelPermission(discord.GetUserData().ID, m_active_channel, Permission::MANAGE_MESSAGES | Permission::MANAGE_CHANNELS)) {
            m_label.set_text("You may bypass slowmode.");
            set_has_tooltip(false);
        } else {
            const auto time_left = GetTimeLeft();
            if (time_left > 0)
                m_label.set_text(std::to_string(time_left) + "s");
            else
                m_label.set_text("");
            set_tooltip_text("Slowmode is enabled. Members can send one message every " + std::to_string(rate_limit) + " seconds.");
        }
    } else {
        m_img.hide();

        m_label.set_text("");
        set_has_tooltip(false);
    }

    if (m_connection)
        m_connection.disconnect();
    m_connection = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &RateLimitIndicator::UpdateIndicator), 1);

    return false;
}

void RateLimitIndicator::OnMessageCreate(const Message &message) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (message.Author.ID != discord.GetUserData().ID) return;
    const bool can_bypass = discord.HasAnyChannelPermission(discord.GetUserData().ID, m_active_channel, Permission::MANAGE_MESSAGES | Permission::MANAGE_CHANNELS);
    if (GetRateLimit() > 0 && !can_bypass) {
        m_times[message.ChannelID] = std::chrono::steady_clock::now();
        UpdateIndicator();
    }
}
