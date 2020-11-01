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
    Snowflake ID;                      //
    std::string Name;                  //
    std::string Icon;                  // null
    std::string Splash;                // null
    std::string DiscoverySplash;       // opt, null (docs wrong)
    bool IsOwner = false;              // opt
    Snowflake OwnerID;                 //
    int Permissions = 0;               // opt
    std::string PermissionsNew;        // opt
    std::string VoiceRegion;           // opt
    Snowflake AFKChannelID;            // null
    int AFKTimeout;                    //
    bool IsEmbedEnabled = false;       // opt, deprecated
    Snowflake EmbedChannelID;          // opt, null, deprecated
    int VerificationLevel;             //
    int DefaultMessageNotifications;   //
    int ExplicitContentFilter;         //
    std::vector<Role> Roles;           //
    std::vector<Emoji> Emojis;         //
    std::vector<std::string> Features; //
    int MFALevel;                      //
    Snowflake ApplicationID;           // null
    bool IsWidgetEnabled = false;      // opt
    Snowflake WidgetChannelID;         // opt, null
    Snowflake SystemChannelID;         // null
    int SystemChannelFlags;            //
    Snowflake RulesChannelID;          // null
    std::string JoinedAt;              // opt*
    bool IsLarge = false;              // opt*
    bool IsUnavailable = false;        // opt*
    int MemberCount = 0;               // opt*
    // std::vector<VoiceStateData> VoiceStates; // opt*
    // std::vector<MemberData> Members; // opt* - incomplete anyways
    std::vector<Channel> Channels; // opt*
    // std::vector<PresenceUpdateData> Presences; // opt*
    int MaxPresences = 0;             // opt, null
    int MaxMembers = 0;               // opt
    std::string VanityURL;            // null
    std::string Description;          // null
    std::string BannerHash;           // null
    int PremiumTier;                  //
    int PremiumSubscriptionCount = 0; // opt
    std::string PreferredLocale;      //
    Snowflake PublicUpdatesChannelID; // null
    int MaxVideoChannelUsers = 0;     // opt
    int ApproximateMemberCount = 0;   // opt
    int ApproximatePresenceCount = 0; // opt

    // undocumented
    // std::map<std::string, Unknown> GuildHashes;
    bool IsLazy = false;

    // * - documentation says only sent in GUILD_CREATE, but these can be sent anyways in the READY event

    friend void from_json(const nlohmann::json &j, Guild &m);
    void update_from_json(const nlohmann::json &j);

    bool HasIcon() const;
    std::string GetIconURL(std::string ext = "png", std::string size = "32") const;
    std::vector<Snowflake> GetSortedChannels(Snowflake ignore = Snowflake::Invalid) const;
};
