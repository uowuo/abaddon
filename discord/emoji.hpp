#pragma once
#include <string>
#include <vector>
#include "json.hpp"
#include "snowflake.hpp"
#include "user.hpp"

struct Emoji {
    Snowflake ID;     // null
    std::string Name; // null (in reactions)
    std::optional<std::vector<Snowflake>> Roles;
    std::optional<User> Creator; // only reliable to access ID
    std::optional<bool> NeedsColons;
    std::optional<bool> IsManaged;
    std::optional<bool> IsAnimated;
    std::optional<bool> IsAvailable;

    friend void from_json(const nlohmann::json &j, Emoji &m);

    std::string GetURL() const;
    static std::string URLFromID(std::string emoji_id);
};
