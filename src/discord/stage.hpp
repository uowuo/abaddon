#pragma once

#include <nlohmann/json.hpp>

#include "snowflake.hpp"

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

struct StageInstance {
    Snowflake ID;
    Snowflake GuildID;
    Snowflake ChannelID;
    std::string Topic;
    StagePrivacy PrivacyLevel;
    Snowflake GuildScheduledEventID;

    friend void from_json(const nlohmann::json &j, StageInstance &m);
};
