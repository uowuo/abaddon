#pragma once
#include "json.hpp"
#include "user.hpp"

enum class RelationshipType {
    None = 0,
    Friend = 1,
    Blocked = 2,
    PendingIncoming = 3,
    PendingOutgoing = 4,
    Implicit = 5,
};

struct RelationshipData {
    // Snowflake UserID; this is the same as ID apparently but it looks new so i wont touch it
    RelationshipType Type;
    Snowflake ID;
    // Unknown Nickname; // null

    friend void from_json(const nlohmann::json &j, RelationshipData &m);
};
