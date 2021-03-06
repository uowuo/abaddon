#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include "user.hpp"
#include "permissions.hpp"
#include <string>
#include <vector>

enum class ChannelType : int {
    GUILD_TEXT = 0,
    DM = 1,
    GUILD_VOICE = 2,
    GROUP_DM = 3,
    GUILD_CATEGORY = 4,
    GUILD_NEWS = 5,
    GUILD_STORE = 6,
    /* 7 and 8 were used for LFG */
    /* 9 and 10 were used for threads */
    PUBLIC_THREAD = 11,
    PRIVATE_THREAD = 12,
    GUILD_STAGE_VOICE = 13,
};

enum class StagePrivacy {
    PUBLIC = 1,
    GUILD_ONLY = 2,
};

constexpr const char *GetStagePrivacyDisplayString(StagePrivacy e) {
    switch (e) {
        case StagePrivacy::PUBLIC:
            return "Public";
        case StagePrivacy::GUILD_ONLY:
            return "Guild Only";
        default:
            return "Unknown";
    }
}

struct ChannelData {
    Snowflake ID;
    ChannelType Type;
    std::optional<Snowflake> GuildID;
    std::optional<int> Position;
    std::optional<std::vector<PermissionOverwrite>> PermissionOverwrites; // shouldnt be accessed
    std::optional<std::string> Name;                                      // null for dm's
    std::optional<std::string> Topic;                                     // null
    std::optional<bool> IsNSFW;
    std::optional<Snowflake> LastMessageID; // null
    std::optional<int> Bitrate;
    std::optional<int> UserLimit;
    std::optional<int> RateLimitPerUser;
    std::optional<std::vector<UserData>> Recipients; // only access id
    std::optional<std::vector<Snowflake>> RecipientIDs;
    std::optional<std::string> Icon; // null
    std::optional<Snowflake> OwnerID;
    std::optional<Snowflake> ApplicationID;
    std::optional<Snowflake> ParentID;           // null
    std::optional<std::string> LastPinTimestamp; // null

    friend void from_json(const nlohmann::json &j, ChannelData &m);
    void update_from_json(const nlohmann::json &j);

    std::optional<PermissionOverwrite> GetOverwrite(Snowflake id) const;
    std::vector<UserData> GetDMRecipients() const;
};
