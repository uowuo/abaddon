#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include "permissions.hpp"
#include <string>
#include <cstdint>

struct RoleData {
    Snowflake ID;
    std::string Name;
    int Color;
    bool IsHoisted;
    int Position;
    int PermissionsLegacy;
    Permission Permissions;
    bool IsManaged;
    bool IsMentionable;

    [[nodiscard]] bool HasColor() const noexcept;
    [[nodiscard]] Glib::ustring GetEscapedName() const;

    friend void from_json(const nlohmann::json &j, RoleData &m);
};
