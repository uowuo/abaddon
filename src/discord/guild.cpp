#include "guild.hpp"

void from_json(const nlohmann::json &j, GuildData &m) {
    JS_D("id", m.ID);
    if (j.contains("unavailable")) {
        m.IsUnavailable = true;
        return;
    }

    JS_D("name", m.Name);
    JS_N("icon", m.Icon);
    JS_N("splash", m.Splash);
    JS_ON("discovery_splash", m.DiscoverySplash);
    JS_O("owner", m.IsOwner);
    JS_O("owner_id", m.OwnerID);
    std::optional<std::string> tmp;
    JS_O("permissions", tmp);
    if (tmp.has_value())
        m.Permissions = std::stoull(*tmp);
    JS_O("region", m.VoiceRegion);
    JS_ON("afk_channel_id", m.AFKChannelID);
    JS_O("afk_timeout", m.AFKTimeout);
    JS_O("embed_enabled", m.IsEmbedEnabled);
    JS_ON("embed_channel_id", m.EmbedChannelID);
    JS_O("verification_level", m.VerificationLevel);
    JS_O("default_message_notifications", m.DefaultMessageNotifications);
    JS_O("explicit_content_filter", m.ExplicitContentFilter);
    JS_O("roles", m.Roles);
    JS_O("emojis", m.Emojis);
    JS_O("features", m.Features);
    JS_O("mfa_level", m.MFALevel);
    JS_ON("application_id", m.ApplicationID);
    JS_O("widget_enabled", m.IsWidgetEnabled);
    JS_ON("widget_channel_id", m.WidgetChannelID);
    JS_ON("system_channel_id", m.SystemChannelID);
    JS_O("system_channel_flags", m.SystemChannelFlags);
    JS_ON("rules_channel_id", m.RulesChannelID);
    JS_O("joined_at", m.JoinedAt);
    JS_O("large", m.IsLarge);
    JS_O("unavailable", m.IsUnavailable);
    JS_O("member_count", m.MemberCount);
    // JS_O("voice_states", m.VoiceStates);
    // JS_O("members", m.Members);
    JS_O("channels", m.Channels);
    JS_O("threads", m.Threads);
    // JS_O("presences", m.Presences);
    JS_ON("max_presences", m.MaxPresences);
    JS_O("max_members", m.MaxMembers);
    JS_ON("vanity_url_code", m.VanityURL);
    JS_ON("description", m.Description);
    JS_ON("banner", m.BannerHash);
    JS_O("premium_tier", m.PremiumTier);
    JS_O("premium_subscription_count", m.PremiumSubscriptionCount);
    JS_O("preferred_locale", m.PreferredLocale);
    JS_ON("public_updates_channel_id", m.PublicUpdatesChannelID);
    JS_O("max_video_channel_users", m.MaxVideoChannelUsers);
    JS_O("approximate_member_count", tmp);
    if (tmp.has_value())
        m.ApproximateMemberCount = std::stol(*tmp);
    JS_O("approximate_presence_count", tmp);
    if (tmp.has_value())
        m.ApproximatePresenceCount = std::stol(*tmp);
}

void GuildData::update_from_json(const nlohmann::json &j) {
    if (j.contains("unavailable")) {
        IsUnavailable = true;
        return;
    }

    JS_RD("name", Name);
    JS_RD("icon", Icon);
    JS_RD("splash", Splash);
    JS_RD("discovery_splash", DiscoverySplash);
    JS_RD("owner", IsOwner);
    JS_RD("owner_id", OwnerID);
    std::string tmp;
    JS_RD("permissions", tmp);
    if (!tmp.empty())
        Permissions = std::stoull(tmp);
    JS_RD("region", VoiceRegion);
    JS_RD("afk_channel_id", AFKChannelID);
    JS_RD("afk_timeout", AFKTimeout);
    JS_RD("embed_enabled", IsEmbedEnabled);
    JS_RD("embed_channel_id", EmbedChannelID);
    JS_RD("verification_level", VerificationLevel);
    JS_RD("default_message_notifications", DefaultMessageNotifications);
    JS_RD("explicit_content_filter", ExplicitContentFilter);
    JS_RD("roles", Roles);
    JS_RD("emojis", Emojis);
    JS_RD("features", Features);
    JS_RD("mfa_level", MFALevel);
    JS_RD("application_id", ApplicationID);
    JS_RD("widget_enabled", IsWidgetEnabled);
    JS_RD("widget_channel_id", WidgetChannelID);
    JS_RD("system_channel_id", SystemChannelID);
    JS_RD("system_channel_flags", SystemChannelFlags);
    JS_RD("rules_channel_id", RulesChannelID);
    JS_RD("joined_at", JoinedAt);
    JS_RD("large", IsLarge);
    JS_RD("unavailable", IsUnavailable);
    JS_RD("member_count", MemberCount);
    // JS_O("voice_states", m.VoiceStates);
    // JS_O("members", m.Members);
    JS_RD("channels", Channels);
    // JS_O("presences", m.Presences);
    JS_RD("max_presences", MaxPresences);
    JS_RD("max_members", MaxMembers);
    JS_RD("vanity_url_code", VanityURL);
    JS_RD("description", Description);
    JS_RD("banner", BannerHash);
    JS_RD("premium_tier", PremiumTier);
    JS_RD("premium_subscription_count", PremiumSubscriptionCount);
    JS_RD("preferred_locale", PreferredLocale);
    JS_RD("public_updates_channel_id", PublicUpdatesChannelID);
    JS_RD("max_video_channel_users", MaxVideoChannelUsers);
    JS_RD("approximate_member_count", ApproximateMemberCount);
    JS_RD("approximate_presence_count", ApproximatePresenceCount);
}

bool GuildData::HasFeature(const std::string &search_feature) const {
    if (!Features.has_value()) return false;
    for (const auto &feature : *Features)
        if (search_feature == feature)
            return true;
    return false;
}

bool GuildData::HasIcon() const {
    return !Icon.empty();
}

bool GuildData::HasAnimatedIcon() const {
    return HasIcon() && Icon[0] == 'a' && Icon[1] == '_';
}

std::string GuildData::GetIconURL(const std::string &ext, const std::string &size) const {
    return "https://cdn.discordapp.com/icons/" + std::to_string(ID) + "/" + Icon + "." + ext + "?size=" + size;
}

void from_json(const nlohmann::json &j, GuildApplicationData &m) {
    JS_D("user_id", m.UserID);
    JS_D("guild_id", m.GuildID);
    auto tmp = j.at("application_status").get<std::string_view>();
    if (tmp == "STARTED")
        m.ApplicationStatus = GuildApplicationStatus::STARTED;
    else if (tmp == "PENDING")
        m.ApplicationStatus = GuildApplicationStatus::PENDING;
    else if (tmp == "REJECTED")
        m.ApplicationStatus = GuildApplicationStatus::REJECTED;
    else if (tmp == "APPROVED")
        m.ApplicationStatus = GuildApplicationStatus::APPROVED;
    JS_N("rejection_reason", m.RejectionReason);
    JS_N("last_seen", m.LastSeen);
    JS_N("created_at", m.CreatedAt);
}
