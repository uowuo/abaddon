#include "member.hpp"

#include "abaddon.hpp"

void from_json(const nlohmann::json &j, GuildMember &m) {
    JS_O("user", m.User);
    JS_ON("nick", m.Nickname);
    JS_D("roles", m.Roles);
    JS_D("joined_at", m.JoinedAt);
    JS_ON("premium_since", m.PremiumSince);
    JS_D("deaf", m.IsDeafened);
    JS_D("mute", m.IsMuted);
    JS_O("user_id", m.UserID);
    JS_ON("avatar", m.Avatar);
    JS_O("pending", m.IsPending);
}

std::vector<RoleData> GuildMember::GetSortedRoles() const {
    std::vector<RoleData> roles;
    for (const auto role_id : Roles) {
        const auto role = Abaddon::Get().GetDiscordClient().GetRole(role_id);
        if (!role.has_value()) continue;
        roles.push_back(*role);
    }

    std::sort(roles.begin(), roles.end(), [](const RoleData &a, const RoleData &b) {
        return a.Position > b.Position;
    });

    return roles;
}

void GuildMember::update_from_json(const nlohmann::json &j) {
    JS_RD("roles", Roles);
    JS_RD("user", User);
    JS_RD("nick", Nickname);
    JS_RD("joined_at", JoinedAt);
    JS_RD("premium_since", PremiumSince);
    JS_RD("avatar", Avatar);
    JS_RD("pending", IsPending);
}
