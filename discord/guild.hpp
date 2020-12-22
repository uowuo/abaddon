#pragma once
#include "json.hpp"
#include "snowflake.hpp"
#include "role.hpp"
#include "channel.hpp"
#include "emoji.hpp"
#include <vector>
#include <string>

// a bot is apparently only supposed to receive the `id` and `unavailable` as false
// but user tokens seem to get the full objects (minus users)
struct Guild {
    Snowflake ID;
    std::string Name;
    std::string Icon;                           // null
    std::string Splash;                         // null
    std::optional<std::string> DiscoverySplash; // null
    std::optional<bool> IsOwner;
    Snowflake OwnerID;
    std::optional<uint64_t> Permissions;
    std::optional<std::string> PermissionsNew;
    std::optional<std::string> VoiceRegion;
    Snowflake AFKChannelID; // null
    int AFKTimeout;
    std::optional<bool> IsEmbedEnabled;      // deprecated
    std::optional<Snowflake> EmbedChannelID; // null, deprecated
    int VerificationLevel;
    int DefaultMessageNotifications;
    int ExplicitContentFilter;
    std::vector<Role> Roles; // only access id
    std::vector<Emoji> Emojis; // only access id
    std::vector<std::string> Features;
    int MFALevel;
    Snowflake ApplicationID; // null
    std::optional<bool> IsWidgetEnabled;
    std::optional<Snowflake> WidgetChannelID; // null
    Snowflake SystemChannelID;                // null
    int SystemChannelFlags;
    Snowflake RulesChannelID;            // null
    std::optional<std::string> JoinedAt; // *
    std::optional<bool> IsLarge;         // *
    std::optional<bool> IsUnavailable;   // *
    std::optional<int> MemberCount;      // *
    // std::vector<VoiceStateData> VoiceStates; // opt*
    // std::vector<MemberData> Members; // opt* - incomplete anyways
    std::optional<std::vector<Channel>> Channels; // *
    // std::vector<PresenceUpdateData> Presences; // opt*
    std::optional<int> MaxPresences; // null
    std::optional<int> MaxMembers;
    std::string VanityURL;   // null
    std::string Description; // null
    std::string BannerHash;  // null
    int PremiumTier;
    std::optional<int> PremiumSubscriptionCount;
    std::string PreferredLocale;
    Snowflake PublicUpdatesChannelID; // null
    std::optional<int> MaxVideoChannelUsers;
    std::optional<int> ApproximateMemberCount;
    std::optional<int> ApproximatePresenceCount;

    // undocumented
    // std::map<std::string, Unknown> GuildHashes;
    bool IsLazy = false;

    // * - documentation says only sent in GUILD_CREATE, but these can be sent anyways in the READY event

    friend void from_json(const nlohmann::json &j, Guild &m);
    void update_from_json(const nlohmann::json &j);

    bool HasIcon() const;
    bool HasAnimatedIcon() const;
    std::string GetIconURL(std::string ext = "png", std::string size = "32") const;
    std::vector<Snowflake> GetSortedChannels(Snowflake ignore = Snowflake::Invalid) const;
};
