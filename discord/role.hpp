#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include "permissions.hpp"
#include <string>
#include <cstdint>

struct Role {
    Snowflake ID;
    std::string Name;
    int Color;
    bool IsHoisted;
    int Position;
    int PermissionsLegacy;
    Permission Permissions;
    bool IsManaged;
    bool IsMentionable;

    friend void from_json(const nlohmann::json &j, Role &m);
};
