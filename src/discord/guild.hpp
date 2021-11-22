#pragma once
#include "json.hpp"
#include "snowflake.hpp"
#include "role.hpp"
#include "channel.hpp"
#include "emoji.hpp"
#include <vector>
#include <string>
#include <unordered_set>

enum class GuildApplicationStatus {
    STARTED,
    PENDING,
    REJECTED,
    APPROVED,
    UNKNOWN,
};

struct GuildApplicationData {
    Snowflake UserID;
    Snowflake GuildID;
    GuildApplicationStatus ApplicationStatus;
    std::optional<std::string> RejectionReason;
    std::optional<std::string> LastSeen;
    std::optional<std::string> CreatedAt;

    friend void from_json(const nlohmann::json &j, GuildApplicationData &m);
};

// a bot is apparently only supposed to receive the `id` and `unavailable` as false
// but user tokens seem to get the full objects (minus users)

// everythings optional cuz of muh partial guild object
// anything not marked optional in https://discord.com/developers/docs/resources/guild#guild-object is guaranteed to be set when returned from Store::GetGuild
struct GuildData {
    Snowflake ID;
    std::string Name;
    std::string Icon;                           // null
    std::string Splash;                         // null
    std::optional<std::string> DiscoverySplash; // null
    std::optional<bool> IsOwner;
    std::optional<Snowflake> OwnerID;
    std::optional<uint64_t> Permissions;
    std::optional<std::string> PermissionsNew;
    std::optional<std::string> VoiceRegion;
    std::optional<Snowflake> AFKChannelID; // null
    std::optional<int> AFKTimeout;
    std::optional<bool> IsEmbedEnabled;      // deprecated
    std::optional<Snowflake> EmbedChannelID; // null, deprecated
    std::optional<int> VerificationLevel;
    std::optional<int> DefaultMessageNotifications;
    std::optional<int> ExplicitContentFilter;
    std::optional<std::vector<RoleData>> Roles;   // only access id
    std::optional<std::vector<EmojiData>> Emojis; // only access id
    std::optional<std::unordered_set<std::string>> Features;
    std::optional<int> MFALevel;
    std::optional<Snowflake> ApplicationID; // null
    std::optional<bool> IsWidgetEnabled;
    std::optional<Snowflake> WidgetChannelID; // null
    std::optional<Snowflake> SystemChannelID; // null
    std::optional<int> SystemChannelFlags;
    std::optional<Snowflake> RulesChannelID; // null
    std::optional<std::string> JoinedAt;     // *
    std::optional<bool> IsLarge;             // *
    std::optional<bool> IsUnavailable;       // *
    std::optional<int> MemberCount;          // *
    // std::vector<VoiceStateData> VoiceStates; // opt*
    // std::vector<MemberData> Members; // opt* - incomplete anyways
    std::optional<std::vector<ChannelData>> Channels; // *
    // std::vector<PresenceUpdateData> Presences; // opt*
    std::optional<int> MaxPresences; // null
    std::optional<int> MaxMembers;
    std::optional<std::string> VanityURL;   // null
    std::optional<std::string> Description; // null
    std::optional<std::string> BannerHash;  // null
    std::optional<int> PremiumTier;
    std::optional<int> PremiumSubscriptionCount;
    std::optional<std::string> PreferredLocale;
    std::optional<Snowflake> PublicUpdatesChannelID; // null
    std::optional<int> MaxVideoChannelUsers;
    std::optional<int> ApproximateMemberCount;
    std::optional<int> ApproximatePresenceCount;
    std::optional<std::vector<ChannelData>> Threads; // only with permissions to view, id only

    // undocumented
    // std::map<std::string, Unknown> GuildHashes;
    bool IsLazy = false;

    // * - documentation says only sent in GUILD_CREATE, but these can be sent anyways in the READY event

    friend void from_json(const nlohmann::json &j, GuildData &m);
    void update_from_json(const nlohmann::json &j);

    bool HasFeature(const std::string &feature);
    bool HasIcon() const;
    bool HasAnimatedIcon() const;
    std::string GetIconURL(std::string ext = "png", std::string size = "32") const;
    std::vector<Snowflake> GetSortedChannels(Snowflake ignore = Snowflake::Invalid) const;
    std::vector<RoleData> FetchRoles() const; // sorted
};
