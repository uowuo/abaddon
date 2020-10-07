#pragma once
#include <string>
#include <vector>
#include "json.hpp"
#include "snowflake.hpp"
#include "user.hpp"

struct Emoji {
    Snowflake ID;                 // null
    std::string Name;             // null (in reactions)
    std::vector<Snowflake> Roles; // opt
    User Creator;                 // opt
    bool NeedsColons = false;     // opt
    bool IsManaged = false;       // opt
    bool IsAnimated = false;      // opt
    bool IsAvailable = false;     // opt

    friend void from_json(const nlohmann::json &j, Emoji &m);

    std::string GetURL() const;
    static std::string URLFromID(std::string emoji_id);
};
