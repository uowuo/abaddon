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
};

struct Channel {
    Snowflake ID;                                          //
    ChannelType Type;                                      //
    Snowflake GuildID;                                     // opt
    int Position = -1;                                     // opt
    std::vector<PermissionOverwrite> PermissionOverwrites; // opt
    std::string Name;                                      // opt, null (null for dm's)
    std::string Topic;                                     // opt, null
    bool IsNSFW = false;                                   // opt
    Snowflake LastMessageID;                               // opt, null
    int Bitrate = 0;                                       // opt
    int UserLimit = 0;                                     // opt
    int RateLimitPerUser = 0;                              // opt
    std::vector<User> Recipients;                          // opt
    std::string Icon;                                      // opt, null
    Snowflake OwnerID;                                     // opt
    Snowflake ApplicationID;                               // opt
    Snowflake ParentID;                                    // opt, null
    std::string LastPinTimestamp;                          // opt, can be null even tho docs say otherwise

    friend void from_json(const nlohmann::json &j, Channel &m);
    void update_from_json(const nlohmann::json &j);

    const PermissionOverwrite *GetOverwrite(Snowflake id) const;
};
