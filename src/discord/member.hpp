#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include "user.hpp"
#include "role.hpp"
#include <string>
#include <vector>

struct GuildMember {
    std::optional<UserData> User; // only reliable to access id. only opt in MESSAGE_*
    std::string Nickname;
    std::vector<Snowflake> Roles;
    std::string JoinedAt;
    std::optional<std::string> PremiumSince; // null
    bool IsDeafened;
    bool IsMuted;
    std::optional<Snowflake> UserID; // present in merged_members
    std::optional<bool> IsPending;   // this uses `pending` not `is_pending`

    // undocuemtned moment !!!1
    std::optional<std::string> Avatar;

    [[nodiscard]] std::vector<RoleData> GetSortedRoles() const;

    void update_from_json(const nlohmann::json &j);
    friend void from_json(const nlohmann::json &j, GuildMember &m);
};
