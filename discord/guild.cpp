#include "guild.hpp"
#include "../abaddon.hpp"

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
    std::optional<std::string> tmp;
    JS_O("permissions", tmp);
    if (tmp.has_value())
        m.Permissions = std::stoull(*tmp);
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
    JS_O("approximate_member_count", tmp);
    if (tmp.has_value())
        m.ApproximateMemberCount = std::stol(*tmp);
    JS_O("approximate_presence_count", tmp);
    if (tmp.has_value())
        m.ApproximatePresenceCount = std::stol(*tmp);
}

void Guild::update_from_json(const nlohmann::json &j) {
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
    if (tmp != "")
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

bool Guild::HasIcon() const {
    return Icon != "";
}

std::string Guild::GetIconURL(std::string ext, std::string size) const {
    return "https://cdn.discordapp.com/icons/" + std::to_string(ID) + "/" + Icon + "." + ext + "?size=" + size;
}

std::vector<Snowflake> Guild::GetSortedChannels(Snowflake ignore) const {
    std::vector<Snowflake> ret;

    const auto &discord = Abaddon::Get().GetDiscordClient();
    auto channels = discord.GetChannelsInGuild(ID);

    std::unordered_map<Snowflake, std::vector<Channel>> category_to_channels;
    std::map<int, std::vector<Channel>> position_to_categories;
    std::map<int, std::vector<Channel>> orphan_channels;
    for (const auto &channel_id : channels) {
        const auto data = discord.GetChannel(channel_id);
        if (!data.has_value()) continue;
        if (!data->ParentID->IsValid() && (data->Type == ChannelType::GUILD_TEXT || data->Type == ChannelType::GUILD_NEWS))
            orphan_channels[*data->Position].push_back(*data);
        else if (data->ParentID->IsValid() && (data->Type == ChannelType::GUILD_TEXT || data->Type == ChannelType::GUILD_NEWS))
            category_to_channels[*data->ParentID].push_back(*data);
        else if (data->Type == ChannelType::GUILD_CATEGORY)
            position_to_categories[*data->Position].push_back(*data);
    }

    for (auto &[pos, channels] : orphan_channels) {
        std::sort(channels.begin(), channels.end(), [&](const Channel &a, const Channel &b) -> bool {
            return a.ID < b.ID;
        });
        for (const auto &chan : channels)
            ret.push_back(chan.ID);
    }

    for (auto &[pos, categories] : position_to_categories) {
        std::sort(categories.begin(), categories.end(), [&](const Channel &a, const Channel &b) -> bool {
            return a.ID < b.ID;
        });
        for (const auto &category : categories) {
            ret.push_back(category.ID);
            if (ignore == category.ID) continue; // stupid hack to save me some time
            auto it = category_to_channels.find(category.ID);
            if (it == category_to_channels.end()) continue;
            auto &channels = it->second;
            std::sort(channels.begin(), channels.end(), [&](const Channel &a, const Channel &b) -> bool {
                return a.Position < b.Position;
            });
            for (auto &channel : channels) {
                ret.push_back(channel.ID);
            }
        }
    }

    return ret;
}
