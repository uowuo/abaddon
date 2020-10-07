#include "guild.hpp"

void from_json(const nlohmann::json &j, Guild &m) {
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
    JS_D("owner_id", m.OwnerID);
    std::string tmp;
    JS_O("permissions", tmp);
    if (tmp != "")
        m.Permissions = std::stoull(tmp);
    JS_D("region", m.VoiceRegion);
    JS_N("afk_channel_id", m.AFKChannelID);
    JS_D("afk_timeout", m.AFKTimeout);
    JS_O("embed_enabled", m.IsEmbedEnabled);
    JS_ON("embed_channel_id", m.EmbedChannelID);
    JS_D("verification_level", m.VerificationLevel);
    JS_D("default_message_notifications", m.DefaultMessageNotifications);
    JS_D("explicit_content_filter", m.ExplicitContentFilter);
    JS_D("roles", m.Roles);
    JS_D("emojis", m.Emojis);
    JS_D("features", m.Features);
    JS_D("mfa_level", m.MFALevel);
    JS_N("application_id", m.ApplicationID);
    JS_O("widget_enabled", m.IsWidgetEnabled);
    JS_ON("widget_channel_id", m.WidgetChannelID);
    JS_N("system_channel_id", m.SystemChannelID);
    JS_D("system_channel_flags", m.SystemChannelFlags);
    JS_N("rules_channel_id", m.RulesChannelID);
    JS_O("joined_at", m.JoinedAt);
    JS_O("large", m.IsLarge);
    JS_O("unavailable", m.IsUnavailable);
    JS_O("member_count", m.MemberCount);
    // JS_O("voice_states", m.VoiceStates);
    // JS_O("members", m.Members);
    JS_O("channels", m.Channels);
    // JS_O("presences", m.Presences);
    JS_ON("max_presences", m.MaxPresences);
    JS_O("max_members", m.MaxMembers);
    JS_N("vanity_url_code", m.VanityURL);
    JS_N("description", m.Description);
    JS_N("banner", m.BannerHash);
    JS_D("premium_tier", m.PremiumTier);
    JS_O("premium_subscription_count", m.PremiumSubscriptionCount);
    JS_D("preferred_locale", m.PreferredLocale);
    JS_N("public_updates_channel_id", m.PublicUpdatesChannelID);
    JS_O("max_video_channel_users", m.MaxVideoChannelUsers);
    JS_O("approximate_member_count", m.ApproximateMemberCount);
    JS_O("approximate_presence_count", m.ApproximatePresenceCount);
}

bool Guild::HasIcon() const {
    return Icon != "";
}

std::string Guild::GetIconURL(std::string ext, std::string size) const {
    return "https://cdn.discordapp.com/icons/" + std::to_string(ID) + "/" + Icon + "." + ext + "?size=" + size;
}
