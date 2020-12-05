#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include "user.hpp"
#include <string>
#include <vector>

struct GuildMember {
    std::optional<User> User; // only reliable to access id. only opt in MESSAGE_*
    std::string Nickname;     // null
    std::vector<Snowflake> Roles;
    std::string JoinedAt;
    std::optional<std::string> PremiumSince; // null
    bool IsDeafened;
    bool IsMuted;

    friend void from_json(const nlohmann::json &j, GuildMember &m);
    static GuildMember from_update_json(const nlohmann::json &j);
};
